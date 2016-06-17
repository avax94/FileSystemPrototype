#include "KernelFS.h"
#include <bitset>
#include <limits.h>
#include "part.h"
#include "KernelFile.h"
#include <vector>
#include "Monitor.h"
#include "LRUCache.h"
#include <iostream>
#include "bitvector.h"
#include "FileHandle.h"
#include "FileSystemPartition.h"

using namespace std;

void initialize()
{
	for (int i = 0; i < 26; i++)
	{
		string mutexName = "Partition" + i;
		std::wstring stemp = std::wstring(mutexName.begin(), mutexName.end());
		LPCWSTR sw = stemp.c_str();
		FileSystemPartition::handles[i] = CreateSemaphore(0, 1, 32, 0);
	}

	for (int i = 0; i < NumPartitions; i++)
	{
		FileSystemPartition::partitions[i] = nullptr;
	}
}

void finilize()
{
	for (int i = 0; i < 26; i++)
	{
		CloseHandle(FileSystemPartition::handles[i]);
	}
	
	delete FileSystemPartition::partitions;
	delete FileSystemPartition::handles;
}

KernelFS::KernelFS()
{
	srw = new SRWLOCK(SRWLOCK_INIT);
	handle = CreateSemaphore(0, 1, 1, 0);
}

void KernelFS::writeEntry(int offset, Entry entry, ClusterNo clusterNo, FileSystemPartition * p)
{
	ReleaseSRWLockShared(srw);
	AcquireSRWLockExclusive(srw);

	int i = 0;
	int wcnt = 0;

	char buff[sizeof(Entry)];
	
	while (entry.name[i] != '\0')
	{		
		buff[i++] = entry.name[i];
	}

	while (i < 8)
	{
		buff[i++] = entry.name[i];
	}

	for (int i = 8; i < 11; i++)
	{
		buff[i] = entry.ext[i - 8];
	}

	buff[11] = 0;

	char* idxCl = (char*)&entry.indexCluster;

	for (int i = 12; i < 16; i++)
	{
		buff[i] = idxCl[i - 12];
	}

	char* sz = (char*)&entry.size;

	for (int i = 16; i < 20; i++)
	{
		buff[i] = sz[i - 16];
	}

	p->write(clusterNo, sizeof(Entry), offset, buff);

	ReleaseSRWLockExclusive(srw);
	AcquireSRWLockShared(srw);
}

char KernelFS::format(char part)
{
	//Check for error in input - lower to upper, rest return 0
	if (part >= 'a' && part <= 'z')
	{
		part = part - 'a' + 'A';
	}
	else if (part<'A' || part>'Z')
	{
		return 0;
	}

	int idx = part - 'A';
	//int idx = part - 1;

	WaitForSingleObject(FileSystemPartition::handles[idx], INFINITE);
	FileSystemPartition* partition = FileSystemPartition::partitions[idx];

	partition->monitor->startFormating();
	//Check if partition isnt mounted
	if (partition == nullptr)
	{
		ReleaseSemaphore(FileSystemPartition::handles[idx], 1, 0);
		return 0;
	}

	bitvector* bitv = partition->getBitvector();

	//Start initializing
	bitv->reset();

	partition->rootIndex = bitv->getFirstFree();
	char writeRootIdx[ClusterSize];
	
	for (int i = 0; i < ClusterSize; i++)
	{
		writeRootIdx[i] = 0;
	}

	partition->write(partition->rootIndex, ClusterSize, 0, writeRootIdx);
	bitv->set(partition->rootIndex);
	//TODO: Maybe we dont need to flush here
	while (bitv->flush() == 0)
	{
		partition->monitor->endFormating();
		ReleaseSemaphore(FileSystemPartition::handles[idx], 1, 0);
		return 0;
	}

	partition->monitor->endFormating();
	ReleaseSemaphore(FileSystemPartition::handles[idx], 1, 0);

	return 1;
}

KernelFile* KernelFS::open(char* fname, char mode)
{
	char part = *fname;
	char lower = tolower(part);

	FileSystemPartition* fp = FileSystemPartition::partitions[lower-'a'];
	if (fp->monitor->open())
	{
		//TADA SYNC
		bool toRead = false;
		if (mode == 'r')
		{
			toRead = true;
			Monitor::startRead(fname);
		}
		else if (mode == 'w' || mode == 'a')
		{
			Monitor::startWrite(fname);
		}
		else
		{
			fp->monitor->close();
			return nullptr;
		}

		Monitor::wMutEx(fname);
		FileHandle* fhandle = doesExist(fname);
		
		if (fhandle != nullptr)
		{
			KernelFile* kf = new KernelFile(fp, fhandle->entry.indexCluster, toRead, fname, fhandle->entry.size, fhandle);
			if (mode == 'w')
			{
				kf->truncate();
			}
			else if(mode == 'a')
			{
				kf->seek(kf->size);
			}

			Monitor::sigMutEx(fname);
			return kf;
		}

		Entry entry;
		int nameCnt = 0;

		while (fname[nameCnt++] != '\0');

		for (int i = 0; i < 3; i++)
		{
			entry.ext[i] = fname[nameCnt - 4 + i];
		}

		int k = 0;

		while (k<8 && fname[k + 3] != '.' && fname[k+3] != '\0')
		{
			entry.name[k] = fname[k + 3];
			k++;
		}

		while (k < 8)
		{
			entry.name[k++] = 0;
		}

		entry.size = 0;
		entry.indexCluster = fp->allocateIndex();
		entry.reserved = 0;

		EntryNum n = allocateEntry(fp, entry);
		fhandle = fp->updateFileHandles(fname, entry, n);
		Monitor::sigMutEx(fname);

		KernelFile* result = new KernelFile(fp, entry.indexCluster, toRead, fname, entry.size, fhandle);

		if (mode == 'a')
		{
			result->seek(result->size);
		}

		return result;
	}

	return nullptr;
}

char KernelFS::deleteFile(char * fname)
{
	FileHandle * fhandle = doesExist(fname);
	
	if (fhandle == nullptr)
	{
		return 0;
	}
	
	KernelFile* f = open(fname, 'w');
	delete f;
	
	char part = *fname;
	char lower = tolower(part);

	FileSystemPartition* fp = FileSystemPartition::partitions[lower - 'a'];
	fp->deleteEntry(fhandle->entryNum);
	

	int i = 0;
	string s = "";

	while (fname[i] != '\0')
	{
		s += fname[i++];
	}

	files.erase(s);

	return 1;
}

EntryNum KernelFS::allocateEntry(FileSystemPartition* p, Entry entry)
{
	char result = 0;
	char* buff = nullptr;
	ClusterNo* currentCluster = new ClusterNo();
	*currentCluster = ULONG_MAX;
	int counter = 0;
	AcquireSRWLockShared(srw);

	while (true)
	{
		if (counter % 102 == 0)
		{
			//TODO checkWhy3;
			*currentCluster = p->getRootClusterNo(counter);
			//*currentCluster = p->getBitvector()->getFirstFree();
			//p->setRootClusterNo(*currentCluster, counter);
		}

		int offset = (counter % 102) * sizeof(Entry);
		char isFree;
		
		p->read(*currentCluster, 1, offset, &isFree);
		if (isFree == 0x00)
		{
			//TODO this syncing is not good - 
			writeEntry(offset, entry, *currentCluster, p);
			break;
		}

		counter++;
	}

	ReleaseSRWLockShared(srw);
	//check
	return counter;
}

char KernelFS::writeSize(char part, int size, char* fname)
{	//TODO CHECK
/*	int idx = tolower(part) - 'a';
	//int idx = part - 1;
	
	FileSystemPartition* p = FileSystemPartition::partitions[idx];

	if (p == nullptr)
	{
		return 0;
	}

	ClusterNo currentCluster = ULONG_MAX;

	char result = 0;
	char* buff = nullptr;
	int counter = 0;
	
	AcquireSRWLockShared(srw);

	while (currentCluster != 0)
	{
		if (counter % 102 == 0)
		{
			//TODO FIRST :: this shouldn't allocate
			currentCluster = p->iterate(counter);
		}

		int offset = (counter % 102) * sizeof(Entry);
		char isFree;
		p->read(currentCluster, 1, offset, &isFree);

		if (isFree != 0x00)
		{
			char* name = parseName(currentCluster, offset, p);
			
			if (strcmp(name, fname) == 0)
			{
				char* sz = (char*)&size;
				ReleaseSRWLockShared(srw);
				AcquireSRWLockExclusive(srw);
				for (int i = 16; i < 20; i++)
				{
					p->write(currentCluster, offset + i, sz[i - 16]);
				}
				ReleaseSRWLockExclusive(srw);
				AcquireSRWLockShared(srw);
				break;
			}
		}

		counter++;
	}
	
	ReleaseSRWLockShared(srw);

	return 1;*/
	return 1;
}

char KernelFS::loadFiles(char part)
{
	//TODO CHECK
	int idx = tolower(part) - 'a';
	//int idx = part - 1;
	FileSystemPartition* p = FileSystemPartition::partitions[idx];


	if (p == nullptr)
	{
		return 0;
	}

	ClusterNo currentCluster = ULONG_MAX;

	char result = 0;
	int counter = 0;

	AcquireSRWLockShared(srw);

	while (currentCluster != 0)
	{
		if (counter % 102 == 0)
		{
			currentCluster = p->iterate(counter);
			
			if (currentCluster == 0)
			{
				break;
			}
		}

		int offset = (counter % 102) * sizeof(Entry);
		char buff[sizeof(Entry)];
		p->read(currentCluster, sizeof(Entry), offset, buff);

		if (buff[0] != 0x00)
		{
			Entry entry = parseEntry(buff);
			char * name = parseName(buff, part);
			p->updateFileHandles(name, entry, counter);
			delete[]name;
		}

		counter++;
	}

	ReleaseSRWLockShared(srw);
	return result;
}

char KernelFS::readRootDir(char part, EntryNum n, Directory &d)
{	
	int idx = tolower(part) - 'a';
	WaitForSingleObject(FileSystemPartition::handles[idx], INFINITE);
	FileSystemPartition* p = FileSystemPartition::partitions[idx];
	ReleaseSemaphore(FileSystemPartition::handles[idx], 1, 0);

	p->monitor->startReadRoot();

	if (p == nullptr)
	{
		return 0;
	}

	ClusterNo currentCluster = ULONG_MAX;

	char result = 0;
	int counter = 0;

	AcquireSRWLockShared(srw);

	while (currentCluster != 0 && result != n)
	{
		if (counter % 102 == 0)
		{
			//TODO FIRST :: this shouldn't allocate
			currentCluster = p->iterate(counter);
		}

		int offset = (counter % 102) * sizeof(Entry);
		char buff[sizeof(Entry)];
		p->read(currentCluster, sizeof(Entry), offset, buff);

		if (buff[0] != 0x00)
		{
			result++;
		}
	
		counter++;
	}

	result = 0;

	while (currentCluster != 0 && result < 65)
	{
		if (counter % 102 == 0)
		{
			currentCluster = p->iterate(counter);
			
			if (currentCluster == 0)
			{
				ReleaseSRWLockShared(srw);
				p->monitor->endReadRoot();
				return result;
			}
		}
		
		int offset = (counter % 102) * sizeof(Entry);

		char buff[sizeof(Entry)];
		p->read(currentCluster, sizeof(Entry), offset, buff);

		if (buff[0] != 0x00)
		{
			d[result++] = parseEntry(buff);
		}

		counter++;
	}

	ReleaseSRWLockShared(srw);

	p->monitor->endReadRoot();
	return result;
}

char* KernelFS::parseName(char * name, char part)
{
	string s;
	
	int i = 0;
	
	while (true)
	{
		if (name[i] != '\0')
		{
			s += name[i++];
		}
		else
		{
			s += '.';
			break;
		}
	}

	for (int i = 8; i < 11; i++)
	{
		s += name[i];
	}

	char* result = new char[s.size() + 4];

	for (int i = 0; i < s.size(); i++)
	{
		result[i+3] = s[i];
	}

	result[0] = part;
	result[1] = ':';
	result[2] = '\\';
	result[s.size()+3] = '\0';

	return result;
}

Entry KernelFS::parseEntry(char* buff)
{
	Entry entry;

	for (int i = 0; i < 8; i++)
	{
		entry.name[i] = buff[i];
	}

	for (int i = 8; i < 11; i++)
	{
		entry.ext[i - 8] = buff[i];
	}

	entry.reserved = 0;

	char* idxCluster = (char*)&entry.indexCluster;

	for (int i = 12; i < 16; i++)
	{
		idxCluster[i-12] = buff[i];
	}

	char* sz = (char*)&entry.size;

	for (int i = 16; i < 20; i++)
	{
		sz[i - 16] = buff[i];
	}

	return entry;
}

void KernelFS::loadFiles(FileSystemPartition* p)
{
	loadFiles(p->getPartitionName() + 'a');
}

FileHandle* KernelFS::doesExist(char *fname)
{
	/*char part = *fname;
	FileSystemPartition *p = FileSystemPartition::partitions[part - 'a'];
	
	char name[8];
	char* fileName = fname + 3;
	int i;

	for (i = 0; i < 8; i++)
	{
		if (fileName[i] == '.')
		{
			break;
		}
		
		name[i] = fileName[i];
	}

	if (i < 8)
	{
		name[i] = '\0';
	}

	Directory d;
	int result = 65;
	EntryNum cnt = 0;
	ClusterNo currentCluster = -1;
	while (currentCluster != 0)
	{
		if (cnt % 102 == 0)
		{
			//TODO FIRST :: this shouldn't allocate
			currentCluster = p->iterate(cnt);
		}

		int offset = (cnt % 102) * sizeof(Entry);
		char buff[sizeof(Entry)];
		p->read(currentCluster, sizeof(Entry), offset, buff);

		if (buff[0] != 0x00)
		{
			result++;
			char* name = parseName(buff, *fname);
			if (strcmp(name, fname) == 0)
			{
				Entry e = parseEntry(buff);
				p->updateFileHandles(fname, e, cnt);
				return &p->files[fname];
			}
		}

		cnt++;
	}

	return nullptr;*/

	char part = *fname;
	FileSystemPartition *partition = FileSystemPartition::partitions[part - 'a'];

	return partition->doesExist(fname);
}

char KernelFS::mount(Partition* part)
{
	int i = 0;
	
	while (i < NumPartitions)
	{
		WaitForSingleObject(FileSystemPartition::handles[i], INFINITE);
		
		if (FileSystemPartition::partitions[i] == nullptr)
		{
			break;
		}

		ReleaseSemaphore(FileSystemPartition::handles[i], 1, 0);
		i++;
	}

	if (i == NumPartitions)
	{
		ReleaseSemaphore(handle, 1, 0);
		return 0;
	}

	FileSystemPartition::partitions[i] = new FileSystemPartition(part, this);
	FileSystemPartition::partitions[i]->monitor = new Monitor(i + 'a');
	ReleaseSemaphore(FileSystemPartition::handles[i], 1, 0);

	return i + 'a';

}

char KernelFS::unmount(char part)
{
	int idx = part - 'a';
	//TODO: CHANGE FORMATTING WITH UNMOUNTING HERE
	WaitForSingleObject(FileSystemPartition::handles[idx], INFINITE);
	FileSystemPartition *fs = FileSystemPartition::partitions[idx];
	ReleaseSemaphore(FileSystemPartition::handles[idx], 1, 0);

	if (fs == nullptr)
	{
		ReleaseSemaphore(handle, 1, 0);
		return 0;
	}

	fs->monitor->startFormating();
	
	WaitForSingleObject(FileSystemPartition::handles[idx], INFINITE);
	FileSystemPartition::partitions[idx] = nullptr;
	ReleaseSemaphore(FileSystemPartition::handles[idx], 1, 0);
	
	delete fs;
	return 1;
}

KernelFS::~KernelFS()
{
	//TODO IMPLEMENT
}