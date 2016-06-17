#pragma once
#include "fs.h"

struct FileHandle
{
	Entry entry;
	EntryNum entryNum;
	char partition;

	FileHandle(char part, Entry e, EntryNum n);
	FileHandle();
	~FileHandle();
};

