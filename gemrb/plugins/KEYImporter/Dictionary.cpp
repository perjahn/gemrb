/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/KEYImporter/Dictionary.cpp,v 1.12 2004/05/25 16:16:35 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "../../includes/globals.h"
#include "Dictionary.h"

//#define THIS_FILE "dictionary.cpp"

/*#define MYASSERT(f) \
  if(!(f))  \
  {  \
  printf("Assertion failed: %s %d",THIS_FILE, __LINE__); \
		abort(); \
  }*/

/////////////////////////////////////////////////////////////////////////////
// inlines
inline unsigned int Dictionary::MyHashKey(const char* key, unsigned int type) const
{
	unsigned int nHash = type;
	for (int i = 0; i < 8 && key[i]; i++) {
		nHash = ( nHash << 5 ) + nHash + toupper( key[i] );
	}
	return nHash;
}
inline int Dictionary::GetCount() const
{
	return m_nCount;
}
inline bool Dictionary::IsEmpty() const
{
	return m_nCount == 0;
}
/////////////////////////////////////////////////////////////////////////////
// out of lines
Dictionary::Dictionary(int nBlockSize, int nHashTableSize)
{
	MYASSERT( nBlockSize > 0 );
	MYASSERT( nHashTableSize > 16 );

	m_pHashTable = NULL;
	m_nHashTableSize = nHashTableSize;  // default size
	m_nCount = 0;
	m_pFreeList = NULL;
	m_pBlocks = NULL;
	m_nBlockSize = nBlockSize;
}

void Dictionary::InitHashTable(unsigned int nHashSize, bool bAllocNow)
	//
	// Used to force allocation of a hash table or to override the default
	//   hash table size of (which is fairly small)
{
	MYASSERT( m_nCount == 0 );
	MYASSERT( nHashSize > 16 );

	if (m_pHashTable != NULL) {
		// free hash table
		delete[] m_pHashTable;
		m_pHashTable = NULL;
	}

	if (bAllocNow) {
		m_pHashTable = new MyAssoc * [nHashSize];
		memset( m_pHashTable, 0, sizeof( MyAssoc * ) * nHashSize );
	}
	m_nHashTableSize = nHashSize;
}

void Dictionary::RemoveAll()
{
	if (m_pHashTable != NULL) {
		// destroy elements (values and keys)
		for (unsigned int nHash = 0; nHash < m_nHashTableSize; nHash++) {
			MyAssoc* pAssoc;
			for (pAssoc = m_pHashTable[nHash];
				pAssoc != NULL;
				pAssoc = pAssoc->pNext) {
				delete[] (char *) pAssoc->key;
			}
		}
	}

	// free hash table
	delete[] m_pHashTable;
	m_pHashTable = NULL;

	m_nCount = 0;
	m_pFreeList = NULL;
	MemBlock* p = m_pBlocks;
	while (p != NULL) {
		MemBlock* pNext = p->pNext;
		delete[] p;
		p = pNext;
	}

	m_pBlocks = NULL;
}

Dictionary::~Dictionary()
{
	RemoveAll();
	MYASSERT( m_nCount == 0 );
}

Dictionary::MyAssoc* Dictionary::NewAssoc()
{
	if (m_pFreeList == NULL) {
		// add another block
		Dictionary::MemBlock* newBlock = ( Dictionary::MemBlock* ) new char[m_nBlockSize*sizeof( Dictionary::MyAssoc ) + sizeof( Dictionary::MemBlock )];

		newBlock->pNext = m_pBlocks;
		m_pBlocks = newBlock;

		// chain them into free list
		Dictionary::MyAssoc* pAssoc = ( Dictionary::MyAssoc* )
			( newBlock + 1 );
		// free in reverse order to make it easier to debug
		pAssoc += m_nBlockSize - 1;
		for (int i = m_nBlockSize - 1; i >= 0; i--, pAssoc--) {
			pAssoc->pNext = m_pFreeList;
			m_pFreeList = pAssoc;
		}
	}
	MYASSERT( m_pFreeList != NULL );  // we must have something

	Dictionary::MyAssoc* pAssoc = m_pFreeList;
	m_pFreeList = m_pFreeList->pNext;
	m_nCount++;
	MYASSERT( m_nCount > 0 );  // make sure we don't overflow
	pAssoc->key = NULL;
	return pAssoc;
}

void Dictionary::FreeAssoc(Dictionary::MyAssoc* pAssoc)
{
	delete[] (char *) pAssoc->key;
	pAssoc->pNext = m_pFreeList;
	m_pFreeList = pAssoc;
	m_nCount--;
	MYASSERT( m_nCount >= 0 );  // make sure we don't underflow

	// if no more elements, cleanup completely
	if (m_nCount == 0) {
		RemoveAll();
	}
}

Dictionary::MyAssoc* Dictionary::GetAssocAt(const char* key,
	unsigned int type, unsigned int& nHash) const
	// find association (or return NULL)
{
	nHash = MyHashKey( key, type ) % m_nHashTableSize;

	if (m_pHashTable == NULL) {
		return NULL;
	}

	// see if it exists
	MyAssoc* pAssoc;
	for (pAssoc = m_pHashTable[nHash];
		pAssoc != NULL;
		pAssoc = pAssoc->pNext) {
		if (type == pAssoc->type) {
			if (!strnicmp( pAssoc->key, key, 8 )) {
				return pAssoc;
			}
		}
	}
	return NULL;
}

bool Dictionary::Lookup(const char* key, unsigned int type,
	unsigned long& rValue) const
{
	unsigned int nHash;
	MyAssoc* pAssoc = GetAssocAt( key, type, nHash );
	if (pAssoc == NULL) {
		return false;
	}  // not in map

	rValue = pAssoc->value;
	return true;
}

void Dictionary::SetAt(const char* key, unsigned int type, unsigned long value)
{
	unsigned int nHash;
	MyAssoc* pAssoc;
	if (( pAssoc = GetAssocAt( key, type, nHash ) ) == NULL) {
		if (m_pHashTable == NULL)
			InitHashTable( m_nHashTableSize );

		// it doesn't exist, add a new Association
		pAssoc = NewAssoc();
		pAssoc->key = key;

		// put into hash table
		pAssoc->pNext = m_pHashTable[nHash];
		m_pHashTable[nHash] = pAssoc;
	} else {
		//keep the stuff consistent (we need only one key in case of duplications)
		delete[] (char *) pAssoc->key; 
		pAssoc->key = key;
	}
	pAssoc->type = type;
	pAssoc->value = value;
}

