#pragma once
#include "part.h"
#include <unordered_map>
#include <Windows.h>
#include "fs.h"

struct cacheEntry
{
	cacheEntry()
	{
		//obj = new char[ClusterSize];
		dirty = false;
	}

	cacheEntry(ClusterNo k, char* o)
	{
		key = k;
		//obj = new char[ClusterSize];
		
		for (int i = 0; i < ClusterSize; i++)
		{
			obj[i] = o[i];
		}

		if (k == 3)
		{
			k = 3;
		}
		next = prev = nullptr;
		dirty = false;	
	}
		
	~cacheEntry()
	{
		//delete[]obj;
	}

	void startRead()
	{
		AcquireSRWLockShared(srwLock);
	}

	void startWrite() 
	{
		AcquireSRWLockExclusive(srwLock);
	}
	
	void endRead()
	{
		ReleaseSRWLockShared(srwLock);
	}

	void endWrite()
	{
		ReleaseSRWLockExclusive(srwLock);
	}
	SRWLOCK* srwLock = new SRWLOCK(SRWLOCK_INIT);
	bool dirty;
	ClusterNo key;
	char obj[ClusterSize];
	cacheEntry* next, *prev;
};

class LRUCache
{
private:
	static int _ID;
	int id;
	HANDLE handle;
	Partition* partition;
	std::unordered_map<ClusterNo, cacheEntry*> cache;
	int numEntries;
	int currEntries;

	cacheEntry* head, *tail;
	
	

	void update(ClusterNo);
	void insert(ClusterNo, char*);
public:
	~LRUCache();
	LRUCache(Partition* p, int capacity);
	int cnt = 0;
	int wcnt = 0;
	void flush(ClusterNo clusterNo);
	void setZero(ClusterNo clusterNo);
	void write(ClusterNo clusterNo, BytesCnt offset, ClusterNo data);
	void read(ClusterNo clusterNo, BytesCnt offset, ClusterNo& data);
	BytesCnt read(ClusterNo clusterNo, BytesCnt bCnt, int offset, char * result);
	void write(ClusterNo clusterNo, BytesCnt bCnt, int offset, char * result);
};

