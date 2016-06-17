#pragma once
#include <unordered_map>
#include "part.h"
#include "fs.h"

class FileSystemPartition;

class FileCache
{
	Partition* partition;
	ClusterNo clusterNo;
	ClusterNo fIndex, sIndex;
	char* dataCluster;
	char* firstIndex;
	char* secondIndex;
	bool dirty;
	bool fdirty, sdirty;
	void update(ClusterNo clusterNo);
public:
	void swipe(bool fIndex, int offset, FileSystemPartition* p);
	void updateSecondIndex(ClusterNo clusterNo);
	BytesCnt read(ClusterNo clusterNo, BytesCnt bCnt, int offset, char * result);
	void write(ClusterNo clusterNo, BytesCnt bCnt, int offset, char * result);
	void write(bool fIndex, BytesCnt offset, ClusterNo data);
	void read(bool fIndex, BytesCnt offset, ClusterNo& data);
	FileCache(Partition* p, ClusterNo fIdx);
	void setZeroClusterSecondIndex();
	~FileCache();
};

