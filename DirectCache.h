#pragma once
#include "part.h"
#include "fs.h"
#include "LRUCache.h"

class DirectCache
{
	int capacity;
	cacheEntry* cache;
	Partition * partition;
	HANDLE* sem;
	int update(ClusterNo);
public:


	~DirectCache();
	DirectCache(Partition* p, int capacity);
	int cnt = 0;
	int wcnt = 0;
	void flush(ClusterNo clusterNo);
	void setZero(ClusterNo clusterNo);
	void write(ClusterNo clusterNo, BytesCnt offset, ClusterNo data);
	void read(ClusterNo clusterNo, BytesCnt offset, ClusterNo& data);
	BytesCnt read(ClusterNo clusterNo, BytesCnt bCnt, int offset, char * result);
	void write(ClusterNo clusterNo, BytesCnt bCnt, int offset, char * result);
};

