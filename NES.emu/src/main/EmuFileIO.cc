/*  This file is part of NES.emu.

	NES.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	NES.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with NES.emu.  If not, see <http://www.gnu.org/licenses/> */

#include "EmuFileIO.hh"
#include <imagine/io/api/stdio.hh>

EmuFileIO::EmuFileIO(IO &srcIO)
{
	auto size = srcIO.size();
	auto mmapData = srcIO.mmapConst();
	if(mmapData)
	{
		io.open(mmapData, size);
	}
	else
	{
		auto romData = new char[size]();
		if(srcIO.read(romData, size) != (ssize_t)size)
		{
			failbit = true;
			return;
		}
		io.open(romData, size, [romData](BufferMapIO &){ delete[] romData; });
	}
}

void EmuFileIO::truncate(s32 length)
{
	io.truncate(length);
}

int EmuFileIO::fgetc()
{
	return ::fgetc(io);
}

size_t EmuFileIO::_fread(const void *ptr, size_t bytes)
{
	ssize_t ret = io.read((void*)ptr, bytes);
	if(ret < (ssize_t)bytes)
		failbit = true;
	return ret;
}

int EmuFileIO::fseek(int offset, int origin)
{
	return ::fseek(io, offset, origin);
}

int EmuFileIO::ftell()
{
	return (int)::ftell(io);
}

int EmuFileIO::size()
{
	return io.size();
}
