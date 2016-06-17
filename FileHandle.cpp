#include "FileHandle.h"

FileHandle::FileHandle(char part, Entry e, EntryNum n) : partition(part), entry(e), entryNum(n)
{
}

FileHandle::FileHandle()
{
}

FileHandle::~FileHandle()
{
}
