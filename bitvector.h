#pragma once
#include <bitset>
#include "part.h"
#include <Windows.h>
#define BITSET 0

class FileSystemPartition;

class bitvector
{
private:
	FileSystemPartition* partition;
	char bitsInCharArray[ClusterSize];
	int arraySize;
	bool valid;
	HANDLE handle;
	unsigned int numOfClusters;

public:
	bitvector(FileSystemPartition* p);


	ClusterNo getNumOfClusters() const;
	ClusterNo getFirstFree();
	bool get(ClusterNo clusterNo);
	int set(ClusterNo clusterNo);
	int reset(ClusterNo clusterNo);
	void reset();
	void set();
	int refresh();
	int flush();
	~bitvector();
};

