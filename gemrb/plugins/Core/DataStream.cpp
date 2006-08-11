/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/DataStream.cpp,v 1.21 2006/08/11 23:17:19 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include <ctype.h>
#include "DataStream.h"

static bool EndianSwitch = false;

DataStream::DataStream(void)
{
	Pos = 0;
	Encrypted = false;
}

DataStream::~DataStream(void)
{
}

void DataStream::SetEndianSwitch(int tmp)
{
	EndianSwitch = !! tmp;
}

bool DataStream::IsEndianSwitch()
{
	return EndianSwitch;
}

/** Returns true if the stream is encrypted */
bool DataStream::CheckEncrypted()
{
	ieWord two;
	Seek( 0, GEM_STREAM_START );
	Read( &two, 2 );
	if (two == 0xFFFF) {
		Pos = 0;
		Encrypted = true;
		size -= 2;
		return true;
	}
	Seek( 0, GEM_STREAM_START );
	Encrypted = false;
	return false;
}
/** No descriptions */
void DataStream::ReadDecrypted(void* buf, unsigned int size)
{
	for (unsigned int i = 0; i < size; i++)
		( ( unsigned char * ) buf )[i] ^= GEM_ENCRYPTION_KEY[( Pos + i ) & 63];
}

void DataStream::Rewind()
{
	Seek( Encrypted ? 2 : 0, GEM_STREAM_START );
	Pos = 0;
}

unsigned long DataStream::GetPos() const
{
	return Pos;
}

unsigned long DataStream::Size() const
{
	return size;
}

unsigned long DataStream::Remains() const
{
	return size-Pos;
}

int DataStream::ReadWord(ieWord *dest)
{
	int len = Read(dest, 2);
	if (EndianSwitch) {
		unsigned char tmp;
		tmp=((unsigned char *) dest)[0];
		((unsigned char *) dest)[0]=((unsigned char *) dest)[1];
		((unsigned char *) dest)[1]=tmp;
	}
	return len;
}

int DataStream::WriteWord(ieWord *src)
{
	int len;
	if (EndianSwitch) {
		char tmp[2];
		tmp[0]=((unsigned char *) src)[1];
		tmp[1]=((unsigned char *) src)[0];
		len = Write( tmp, 2 );
	}
	else {
		len = Write( src, 2 );
	}
	return len;
}

int DataStream::ReadDword(ieDword *dest)
{
	int len = Read(dest, 4);
	if (EndianSwitch) {
		unsigned char tmp;
		tmp=((unsigned char *) dest)[0];
		((unsigned char *) dest)[0]=((unsigned char *) dest)[3];
		((unsigned char *) dest)[3]=tmp;
		tmp=((unsigned char *) dest)[1];
		((unsigned char *) dest)[1]=((unsigned char *) dest)[2];
		((unsigned char *) dest)[2]=tmp;
	}
	return len;
}

int DataStream::WriteDword(ieDword *src)
{
	int len;
	if (EndianSwitch) {
		char tmp[4];
		tmp[0]=((unsigned char *) src)[3];
		tmp[1]=((unsigned char *) src)[2];
		tmp[2]=((unsigned char *) src)[1];
		tmp[3]=((unsigned char *) src)[0];
		len = Write( tmp, 4 );
	}
	else {
		len = Write( src, 4 );
	}
	return len;
}

int DataStream::ReadResRef(ieResRef dest)
{
	int len = Read(dest, 8);
	int i;
	//zero filling from the first space or in the end
	//we got a 9 bytes long buffer
	for(i=0;(i<8) && (dest[i]!=' ');i++) dest[i]=(char) tolower(dest[i]);
	while(i<9) dest[i++]=0;
	return len;
}

int DataStream::WriteResRef(const ieResRef src)
{
	return Write( src, 8);
}
