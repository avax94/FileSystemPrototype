#include "Index.h"
#include "FileSystemPartition.h"
/*
Index::Index(const char* buff, FileSystemPartition* part)
{
	partition = part;
	firstLayer = new ClusterNo[ClusterSize];
	ClusterNo* iterator = (ClusterNo*)buff;
	for (int i = 0; i < ClusterSize / sizeof(ClusterNo); i++)
	{
		firstLayer[i] = *iterator;
		iterator++;
	}

	secondLayer = new ClusterNo*[ClusterSize / 2];

	for (int i = ClusterSize / 2; i < ClusterSize; i++)
	{
		char* buff;
		partition->readCluster(firstLayer[i], buff);
		parse(buff, secondLayer[i - ClusterSize / 2]);
	}
}

void Index::parse(const char* buff, ClusterNo* arr)\
{
	arr = new ClusterNo[ClusterSize];
	ClusterNo* iterator = (ClusterNo*)buff;
	for (int i = 0; i < ClusterSize / sizeof(ClusterNo); i++)
	{
		arr[i] = *iterator;
		iterator++;
	}
}*/


