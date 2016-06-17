#pragma once
#include <Windows.h>
#include "fs.h"
#include "part.h"
#include "FileHandle.h"
#include <unordered_map>

using namespace std;

class DirectCache;
class LRUCache;
class bitvector;
class Partition;
class Monitor;


class FileSystemPartition
{
	char partitionName;
	bitvector* _bitvector;
	Partition* _partition;
	
	HANDLE handle;
	static int id;
	int _id;
	unordered_map<string, FileHandle> files;
	friend class KernelFile;
	friend class KernelFS;
	bool filesLoaded = false;

public:
	DirectCache* cache;
	static FileSystemPartition** partitions;
	static HANDLE *handles;
	KernelFS* kernelFS;
	FileHandle* updateFileHandles(char* name, Entry entry, EntryNum n);

	Monitor* monitor;
	ClusterNo rootIndex;

	FileHandle* doesExist(char*);
	ClusterNo iterate(EntryNum);
	ClusterNo allocateIndex(ClusterNo index, int offset);
	ClusterNo allocateIndex();
	ClusterNo allocateDataCluster(ClusterNo index, int offset);
	ClusterNo getRootClusterNo(EntryNum entryNum);
	//TODO: Implement synchronus
	ClusterNo getNumOfClusters() const;
	ClusterNo setFirstFree();
	FileSystemPartition(Partition* p, KernelFS* k);
	~FileSystemPartition();
	ClusterNo getPageNum(ClusterNo index, BytesCnt bytes);
	ClusterNo getClusterNoFromEntryNo(EntryNum entryNum);
	void writeSize(EntryNum n, int size);
	void deleteEntry(EntryNum n);
	bitvector* getBitvector() const;
	char getPartitionName() const;
	int write(ClusterNo clusterNo, BytesCnt bCnt, int offset, char * x);
	int read(ClusterNo clusterNo, BytesCnt bCnt, int offset, char * x);
	char writeToDisk(ClusterNo clstNo, char* buff);
	char readFromDisk(ClusterNo clstNo, char* buff);
	//friend KernelFS;
};


