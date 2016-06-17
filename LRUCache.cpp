#include "LRUCache.h"
#include <string>
#include <iostream>
using namespace std;

int LRUCache::_ID = 0;

LRUCache::LRUCache(Partition* p, int capacity) : partition(p), tail(nullptr), head(nullptr), numEntries(capacity)
{
	id = _ID++;
	currEntries = 0;
	handle = CreateSemaphore(0, 1, 1, 0);
	head = tail = nullptr;
}

void LRUCache::flush(ClusterNo clusterNo)
{
	if (cache[clusterNo]->dirty)
	{
		while(partition->writeCluster(clusterNo, cache[clusterNo]->obj) == 0);
		cache[clusterNo]->dirty = false;
	}
}

void LRUCache::setZero(ClusterNo clusterNo)
{
	WaitForSingleObject(handle, INFINITE);
	update(clusterNo);
	ReleaseSemaphore(handle, 1, 0);

	for (int i = 0; i < ClusterSize; i++)
	{
		cache[clusterNo]->obj[i] = 0;
	}

	cache[clusterNo]->dirty = true;
}

void LRUCache::write(ClusterNo clusterNo, BytesCnt offset, ClusterNo data)
{
	char* d = (char*)&data;
	write(clusterNo, sizeof(ClusterNo), offset, d);
}

void LRUCache::read(ClusterNo clusterNo, BytesCnt offset, ClusterNo & data)
{
	char* d = (char*)&data;
	read(clusterNo, sizeof(ClusterNo), offset, d);
}

LRUCache::~LRUCache()
{
	while (head)
	{
		cacheEntry *old = head;
		head = head->next;
		
		if (old->dirty)
		{
			while(partition->writeCluster(old->key, old->obj) == 0);
		}
		
		delete old;
	}

	CloseHandle(handle);
}

BytesCnt LRUCache::read(ClusterNo clusterNo, BytesCnt bCnt, int offset, char* result)
{
	
	WaitForSingleObject(handle, INFINITE);
	update(clusterNo);
	ReleaseSemaphore(handle, 1, 0);
	
	BytesCnt i;

	cache[clusterNo]->startRead();
	for (i = 0; i < bCnt && offset+i < ClusterSize; i++)
	{
		result[i] = cache[clusterNo]->obj[offset+i];
	}
	cache[clusterNo]->endRead();
	return i;
}

void LRUCache::write(ClusterNo clusterNo, BytesCnt bCnt, int offset, char* result)
{
	WaitForSingleObject(handle, INFINITE);
	update(clusterNo);
	ReleaseSemaphore(handle, 1, 0);

	cache[clusterNo]->startWrite();
	for (int i = 0; i < bCnt && offset + i < ClusterSize; i++)
	{
		cache[clusterNo]->obj[offset + i] = result[i];
	}

	cache[clusterNo]->dirty = true;
	cache[clusterNo]->endWrite();
}

void LRUCache::update(ClusterNo clusterNo)
{
	char *data = nullptr;
	
	if (cache[clusterNo]==nullptr)
	{
		data = new char[ClusterSize];
		while(partition->readCluster(clusterNo, data) == 0);
		insert(clusterNo, data);
		delete[]data;
	}
	else
	{
		cacheEntry* entry = cache[clusterNo];
		if (entry != head)
		{
			if (entry != tail)
			{
				entry->next->prev = entry->prev;
			}
			else
			{
				tail = tail->prev;
			}

			entry->prev->next = entry->next;
			entry->next = head;
			entry->prev = nullptr;
			head->prev = entry;
			head = entry;
		}
	}
}

void LRUCache::insert(ClusterNo clusterNo, char* data)
{
	cacheEntry* entry = new cacheEntry(clusterNo, data);
	entry->prev = nullptr;
	cache[clusterNo] = entry;
	cnt++;
	if (currEntries == 0)
	{
		currEntries++;
		head = tail = entry;
	}
	else if (currEntries < numEntries)
	{
		entry->prev = nullptr;
		entry->next = head;
		head->prev = entry;
		head = entry;

		currEntries++;
	}
	else
	{
		cacheEntry* toBeDeleted = tail;
		tail = tail->prev;

		if (toBeDeleted->dirty)
		{
			//TODO: Check what u meant here seems like a mistake::same shit different story
			while (partition->writeCluster(toBeDeleted->key, toBeDeleted->obj) == 0)
			{
				wcnt++;
			}
		}
		
		cache.erase(toBeDeleted->key);
		delete toBeDeleted;

		tail->next = nullptr;
		entry->prev = nullptr;
		entry->next = head;
		head->prev = entry;
		head = entry;
	}
}



