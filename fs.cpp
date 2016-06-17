#include "fs.h"
#include "KernelFS.h"
#include "File.h"
#include "FileHandle.h"
//TODO destroy
KernelFS* FS::myImpl = new KernelFS();

FS::~FS()
{
	delete myImpl;
}

char FS::mount(Partition * partition)
{
	return myImpl->mount(partition);
}

char FS::unmount(char part)
{
	return myImpl->unmount(part);
}

char FS::format(char part)
{
	return myImpl->format(part);
}

char FS::readRootDir(char part, EntryNum n, Directory & d)
{
	return myImpl->readRootDir(part, n, d);
}

char FS::doesExist(char * fname)
{
	FileHandle* res = myImpl->doesExist(fname);
	
	if (res == nullptr)
	{
		return 0;
	}

	return 1;
}

File * FS::open(char * fname, char mode)
{
	KernelFile* myFile = myImpl->open(fname, mode);
	File* file = new File();
	file->myImpl = myFile;
	return file;
}

char FS::deleteFile(char * fname)
{
	return myImpl->deleteFile(fname);
}


