#include "bitvector.h"
#include "FileSystemPartition.h"
#include <cmath>

using namespace std;

//TODO: Do error checking for reading cluster in bitsInCharArray, if failed try calling it everytime you are trying to read something
bitvector::bitvector(FileSystemPartition* p)
{
	partition = p;
	ClusterNo clusterNo = p->getNumOfClusters();
	arraySize = (int)ceil(clusterNo / 8);
	//TODO: ROUND TO mULTIPLE OF 2048
	numOfClusters = 1;
	p->read(BITSET + numOfClusters, ClusterSize, 0, bitsInCharArray);

	string mutexName = "bitvector" + partition->getPartitionName();
	std::wstring stemp = std::wstring(mutexName.begin(), mutexName.end());
	LPCWSTR sw = stemp.c_str();
	handle = CreateSemaphore(0, 1, 1, 0);

	//TODO: handle this man, come on
	if (handle == nullptr)
	{
		throw new runtime_error("Couldn't create mutex");
	}
}

ClusterNo bitvector::getNumOfClusters() const
{
	return numOfClusters;
}

bool bitvector::get(ClusterNo clusterNo)
{
	if (clusterNo < 0 || clusterNo >= arraySize*8)
	{
		return false;
	}

	if (clusterNo == 125)
	{

	}


	int idx = clusterNo / 8;
	int offset = clusterNo % 8;
	
	short bitgrp = bitsInCharArray[idx];
	bitgrp &= (1 << offset);

	if (bitgrp == 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

ClusterNo bitvector::getFirstFree()
{
	ClusterNo clusterNo = 1;

	ClusterNo numOfClusters = partition->getNumOfClusters();

	WaitForSingleObject(handle, INFINITE);
	while (clusterNo < numOfClusters && get(clusterNo))
	{
		clusterNo++;
	}

	if (clusterNo == numOfClusters)
	{
		ReleaseSemaphore(handle, 1, 0);
		return 0;
	}

	set(clusterNo);

	ReleaseSemaphore(handle, 1, 0);
	return clusterNo;
}

int bitvector::set(ClusterNo clusterNo)
{
	if (clusterNo < 0 || clusterNo >= arraySize*8)
	{
		return 0;
	}

	if (clusterNo == 125)
	{

	}


	int idx = clusterNo / 8;
	int offset = clusterNo % 8;

	short bitgrp = bitsInCharArray[idx];
	bitgrp |= (1 << offset);
	bitsInCharArray[idx] = bitgrp;

	return 1;
}

int bitvector::reset(ClusterNo clusterNo)
{
	if (clusterNo < 0 || clusterNo >= arraySize*8)
	{
		return -1;
	}

	int idx = clusterNo / 8;
	int offset = clusterNo % 8;
	short bitgrp = bitsInCharArray[idx];
	bitgrp &= ~(1 << offset);

	return 1;
}

void bitvector::set()
{
	char toSet = ~0;

	for (int i = 0; i < arraySize; i++)
	{
		bitsInCharArray[i] = toSet;
	}
}

void bitvector::reset()
{
	char toSet = 0;

	for (int i = 0; i < arraySize; i++)
	{
		bitsInCharArray[i] = toSet;
	}

	int limit = arraySize % ClusterSize == 0 ? arraySize / ClusterSize-1 : arraySize / ClusterSize;

	for (int i = 0; i <= limit; i++)
	{
		set(i);
	}
}

int bitvector::flush()
{
	return partition->writeToDisk(BITSET, bitsInCharArray);
}

int bitvector::refresh()
{
	return partition->readFromDisk(BITSET, bitsInCharArray);
}

bitvector::~bitvector()
{
	while(flush()==0);
	//delete[]bitsInCharArray;
}
