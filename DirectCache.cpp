#include "DirectCache.h"
#include <string>
#include <iostream>

using namespace std;

DirectCache::DirectCache(Partition* p, int capacity) : partition(p)
{
	this->capacity = capacity;
	cache = new cacheEntry[capacity];
	sem = new HANDLE[capacity];
	
	for (int i = 0; i < capacity; i++)
	{
		cache[i].dirty = false;
		cache[i].key= -1;
		sem[i] = CreateSemaphore(0, 1, 1, 0);
	}
}

void DirectCache::flush(ClusterNo clusterNo)
{
	int idx = update(clusterNo);

	if (cache[idx].dirty)
	{
		while (partition->writeCluster(clusterNo, cache[idx].obj) == 0);
		cache[idx].dirty = false;
	}
}

void DirectCache::setZero(ClusterNo clusterNo)
{
	int idx = update(clusterNo);

	for (int i = 0; i < ClusterSize; i++)
	{
		cache[idx].obj[i] = 0;
	}

	cache[idx].dirty = true;
}

void DirectCache::write(ClusterNo clusterNo, BytesCnt offset, ClusterNo data)
{
	char* d = (char*)&data;
	write(clusterNo, sizeof(ClusterNo), offset, d);
}

void DirectCache::read(ClusterNo clusterNo, BytesCnt offset, ClusterNo & data)
{
	char* d = (char*)&data;
	read(clusterNo, sizeof(ClusterNo), offset, d);
}

DirectCache::~DirectCache()
{
	for (int i = 0; i < capacity; i++)
	{
		if (cache[i].dirty)
		{
			partition->writeCluster(cache[i].key, cache[i].obj);
		}
	}
	
	delete[]cache;

	for (int i = 0; i < capacity; i++)
	{
		CloseHandle(sem[i]);
	}

	delete[]sem;
}

BytesCnt DirectCache::read(ClusterNo clusterNo, BytesCnt bCnt, int offset, char* result)
{
	int idx = update(clusterNo);

	BytesCnt i;

	cache[idx].startRead();
	for (i = 0; i < bCnt && offset + i < ClusterSize; i++)
	{
		if (offset + i > ClusterSize)
		{
			offset = offset;
		}

		result[i] = cache[idx].obj[offset + i];
	}
	cache[idx].endRead();
	return i;
}

void DirectCache::write(ClusterNo clusterNo, BytesCnt bCnt, int offset, char* result)
{
	int idx = update(clusterNo);

	cache[idx].startWrite();
	for (int i = 0; i < bCnt && offset + i < ClusterSize; i++)
	{
		cache[idx].obj[offset + i] = result[i];
	}

	cache[idx].dirty = true;
	cache[idx].endWrite();
}

int DirectCache::update(ClusterNo clusterNo)
{
	int idx = clusterNo % capacity;

	if (cache[idx].key != clusterNo)
	{
		if (cache[idx].dirty)
		{
			while (partition->writeCluster(cache[idx].key, cache[idx].obj) == 0);
		}

		char data[ClusterSize];

		cache[idx].key = clusterNo;
		cache[idx].dirty = false;
		while (partition->readCluster(clusterNo, cache[idx].obj) == 0);
	}

	return idx;
}
