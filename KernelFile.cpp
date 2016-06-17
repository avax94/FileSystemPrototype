#include "KernelFile.h"
#include "Monitor.h"
#include "KernelFS.h"
#include "testprimer.h"
#include "FileSystemPartition.h"
#include <iostream>
#include "FileCache.h"

#define CLUSTERENTRIES ClusterSize / sizeof(ClusterNo)

KernelFile::KernelFile(FileSystemPartition *p, ClusterNo index, bool r, char* fname, int size, FileHandle * f)
{
	for (int i = 0; fname[i] != '\0'; i++)
	{
		name[i] = fname[i];
	}

	fhandle = f;
	dirty = false;
	toRead = r;
	partition = p;
	this->size = size;
	currentCluster = clusterCursor = cursor = 0;
	this->index = index;
	cache = new FileCache(p->_partition, index);
}

//TODO WRITE ALLOCATED CLUSTER TO FILE INDEX
KernelFile::~KernelFile()
{
	if (toRead)
	{
		Monitor::endRead(name);
	}
	else
	{
		if (dirty)
		{
			fhandle->entry.size = size;
			partition->writeSize(fhandle->entryNum, size);
		}

		Monitor::endWrite(name);
	}
	
	delete cache;

	partition->monitor->close();
}

BytesCnt KernelFile::getFileSize()
{
	return size;
}

char KernelFile::truncate()
{
	size = cursor;
	dirty = true;
	ClusterNo startingCluster = cursor / ClusterSize;
	deallocateClusters(startingCluster);
	return 1;
}

char KernelFile::eof()
{
	return cursor == size ? 1 : 0;
}

char KernelFile::read(char& x)
{
	char result = 0;

	if (eof() == 0)
	{
		updateCurrentCluster();
		cursor++;
		cache->read(currentCluster, 1, cursor % ClusterSize, &x);
	}
	
	return result;
}

char KernelFile::write(char x)
{
	char result = 0;

	if (!toRead)
	{
		//what ?
		updateCurrentCluster();
		cursor++;
		cache->write(currentCluster, 1, cursor % ClusterSize, &x);
	
		if (cursor > size)
		{
			size = cursor;
			dirty = true;
		}
	}

	return result;
}

char KernelFile::seek(BytesCnt bCnt)
{
	if (bCnt > size)
	{
		return 0;
	}
	
	cursor = bCnt;

	return 1;
}

BytesCnt KernelFile::filePos()
{
	return cursor;
}

void KernelFile::updateCurrentCluster()
{
	if (currentCluster == 0 || cursor % ClusterSize == 0)
	{
		currentCluster = getPageNum(cursor);	
	}
}

char KernelFile::write(BytesCnt bCnt, char* buff)
{
	char result = 0;

	BytesCnt leftovers = bCnt;

	if (!toRead)
	{
		char* ptr = buff;
		while (leftovers > 0)
		{
			BytesCnt lefty = ClusterSize - (cursor % ClusterSize);
			BytesCnt toWrite = lefty > leftovers ? leftovers : lefty;
			leftovers -= toWrite;

			updateCurrentCluster();
			
			cache->write(currentCluster, toWrite, cursor % ClusterSize, ptr);
			cursor += toWrite;
			ptr += toWrite;
			if (cursor > size)
			{
				size = cursor;
				dirty = true;
			}
		}
	}

	return result;
}

BytesCnt KernelFile::read(BytesCnt bCnt, char* buffer)
{
	if (bCnt + cursor > size)
	{
		bCnt = size - cursor;
	}

	BytesCnt result = 0;
	BytesCnt leftovers = bCnt;
	char* ptr = buffer;

	while (leftovers > 0)
	{
		BytesCnt lefty = ClusterSize - (cursor % ClusterSize);
		BytesCnt toWrite = lefty > leftovers ? leftovers : lefty;
		leftovers -= toWrite;
		updateCurrentCluster();
		result = cache->read(currentCluster, toWrite, cursor % ClusterSize, ptr);
		cursor += toWrite;
		ptr += toWrite;
	}

	return result;
}


ClusterNo KernelFile::getPageNum(BytesCnt bytes)
{
	//int idx = bytes % ClusterSize != 0 ? bytes / ClusterSize : bytes / ClusterSize - 1;
	int idx = bytes / ClusterSize;
	int numEntries = ClusterSize / sizeof(ClusterNo);

	if (bytes == 0)
	{
		idx = 0;
	}

	ClusterNo result;
	ClusterNo dataCluster;
	int offset = idx;

	if (idx >= numEntries / 2)
	{
		idx -= numEntries / 2;
		//idx = idx % ClusterSize != 0 ? idx / ClusterSize : idx / ClusterSize - 1;
		offset = idx % numEntries;
		idx = idx / numEntries;

		cache->read(true, ClusterSize/2 + idx*sizeof(ClusterNo), result);

		if (result == 0)
		{
			result = allocateIndex((numEntries / 2 + idx)*sizeof(ClusterNo));
		}

		cache->updateSecondIndex(result);
		cache->read(false, offset*sizeof(ClusterNo), dataCluster);
	}
	else
	{
		cache->read(true, offset*sizeof(ClusterNo), dataCluster);
		result = index;
	}

	if (dataCluster == 0)
	{
		dataCluster = allocateDataCluster(result == index, offset*sizeof(ClusterNo));
	}

	return dataCluster;
}

ClusterNo KernelFile::allocateIndex(int offset)
{
	ClusterNo result = partition->setFirstFree();
	cache->updateSecondIndex(result);
	cache->setZeroClusterSecondIndex();
	cache->write(true, offset, result);

	return result;
}

ClusterNo KernelFile::allocateDataCluster(bool findex, int offset)
{
	ClusterNo toAdd = partition->setFirstFree();
	cache->write(findex, offset, toAdd);

	return toAdd;
}

void KernelFile::deallocateClusters(ClusterNo startingCluster) 
{ 
	ClusterNo currentCluster = -1;

	if (startingCluster / CLUSTERENTRIES != 0 && startingCluster % CLUSTERENTRIES != 0)
	{
		cache->read(true, (startingCluster / CLUSTERENTRIES - 1) * sizeof(ClusterNo) + ClusterSize / 2, currentCluster);
		cache->updateSecondIndex(currentCluster);
		cache->swipe(false, (startingCluster % CLUSTERENTRIES) * sizeof(ClusterNo), partition);
		cache->swipe(true, (startingCluster / CLUSTERENTRIES) * sizeof(ClusterNo) + ClusterSize / 2, partition);
	}
	else
	{
		cache->swipe(true, startingCluster * sizeof(ClusterNo), partition);
	}

}