#pragma once
#include "part.h"
#include "fs.h"
#include <windows.h>


struct Cluster {
	ClusterNo clusterNo;
	bool dirty;
	char buff[ClusterSize];
	
	SRWLOCK* srwlock;
	void startRead();
	void startWrite();
	void endRead();
	void endWrite();
	void update(ClusterNo);

	Cluster();
	~Cluster();
};

enum Index
{
	ROOT,
	SECOND,
	DATA
};

class RootCache
{
	Cluster data;
	Cluster rootIndex;
	Cluster secondIndex;
public:
	RootCache();
	~RootCache();
	void flush(Index index);
	void setZero(Index index);
	void write(Index index, ClusterNo clusterNo, BytesCnt offset, ClusterNo data);
	void read(Index index, ClusterNo clusterNo, BytesCnt offset, ClusterNo& data);
	BytesCnt read(Index index, ClusterNo clusterNo, BytesCnt bCnt, int offset, char * result);
	void write(Index index, ClusterNo clusterNo, BytesCnt bCnt, int offset, char * result);
};

