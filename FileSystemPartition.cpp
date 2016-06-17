#include "FileSystemPartition.h"
#include "bitvector.h"
#include "LRUCache.h"
#include "KernelFS.h"
#include "fs.h"
#include "Monitor.h"
#include "LRUCache.h"
#include <string>
#include "DirectCache.h"

#define ENTRYSIZE 20

using namespace std;

int FileSystemPartition::id = 0;
FileSystemPartition** FileSystemPartition::partitions = new FileSystemPartition*[26];
HANDLE* FileSystemPartition::handles = new HANDLE[26];

FileSystemPartition::FileSystemPartition(Partition* p, KernelFS* k)
{
	kernelFS = k;
	_partition = p;
	cache = new DirectCache(p, 100);
	_bitvector = new bitvector(this);
	rootIndex = _bitvector->getNumOfClusters();
	monitor = nullptr;
	_id = id++;

	string mutexName = "handle" + _id;
	std::wstring stemp = std::wstring(mutexName.begin(), mutexName.end());
	LPCWSTR sw = stemp.c_str();
	handle = CreateSemaphore(0, 1, 1, 0);
}

FileSystemPartition::~FileSystemPartition()
{
	delete _bitvector;
	delete monitor;
	WaitForSingleObject(handle, INFINITE);
	delete cache;
	ReleaseSemaphore(handle, 1, 0);
}

char FileSystemPartition::getPartitionName() const
{
	return partitionName;
}

bitvector* FileSystemPartition::getBitvector() const
{
	return _bitvector;
}


int FileSystemPartition::write(ClusterNo clusterNo, BytesCnt bCnt, int offset, char* x)
{
	cache->write(clusterNo, bCnt, offset, x);
	return 1;
}
int FileSystemPartition::read(ClusterNo clusterNo, BytesCnt bCnt, int offset, char* x)
{
	return cache->read(clusterNo, bCnt, offset, x);
}
char FileSystemPartition::writeToDisk(ClusterNo clstNo, char * buff)
{
	while (_partition->writeCluster(clstNo, buff) == 0);

	return 1;
}
char FileSystemPartition::readFromDisk(ClusterNo clstNo, char * buff)
{
	while (_partition->readCluster(clstNo, buff) == 0);

	return 1;
}

ClusterNo FileSystemPartition::allocateIndex(ClusterNo index, int offset)
{
	ClusterNo result = _bitvector->getFirstFree();
	cache->setZero(result);
	cache->write(index, offset, result);

	return result;
}
ClusterNo FileSystemPartition::allocateIndex()
{
	ClusterNo result = _bitvector->getFirstFree();
	cache->setZero(result);
	cache->flush(result);
	return result;
}

FileHandle* FileSystemPartition::updateFileHandles(char* name, Entry entry, EntryNum n)
{
	int i = 0;
	string s = "";
	
	while (name[i] != '\0')
	{
		s += name[i++];
	}

	if (files.find(s) == files.end())
	{
		files[s] = FileHandle(partitionName, entry, n);
	}

	return &files[s];
}
FileHandle* FileSystemPartition::doesExist(char * name)
{
	if (!filesLoaded)
	{
		kernelFS->loadFiles(this);
		filesLoaded = true;
	}

	int i = 0;
	string s = "";
	
	while (name[i] != '\0')
	{
		s += name[i++];
	}

	if (files.find(s) != files.end())
	{
		return &files[s];
	}
	else
	{
		return nullptr;
	}
}
//TODO CHECK THIS
ClusterNo FileSystemPartition::iterate(EntryNum entryNum)
{
	int numOfEntriesInSingleClusterHehehehehehehe = ClusterSize / ENTRYSIZE;
	//int idx = entryNum % numOfEntriesInSingleClusterHehehehehehehe == 0 ? entryNum / numOfEntriesInSingleClusterHehehehehehehe - 1 : entryNum / numOfEntriesInSingleClusterHehehehehehehe;
	int idx = entryNum / numOfEntriesInSingleClusterHehehehehehehe;
	int offset = idx;

	////WaitForSingleObject(handle, INFINITE);
	//you are making copy here
	//char* buff = cache->read(rootIndex);
	ClusterNo writingCluster = rootIndex;

	ClusterNo result = rootIndex;// *(((ClusterNo*)buff) + idx);

	if (idx >= ClusterSize / 2)
	{
		idx -= ClusterSize / 2;
		offset = idx % ClusterSize;
		idx = idx % ClusterSize != 0 ? idx / ClusterSize : idx / ClusterSize - 1;

		//TODO allocating and writing should be synchronus
		//result = writingCluster = *(((ClusterNo*)buff) + idx + ClusterSize / 2);
		cache->read(rootIndex, (idx + ClusterSize / 2)*sizeof(ClusterNo), result);

		if (result == 0)
		{
			//ReleaseSemaphore(handle, 1, 0);
			return 0;
		}

		 cache->read(result, offset*sizeof(ClusterNo), result);
	}
	else
	{
		cache->read(rootIndex, idx*sizeof(ClusterNo), result);
	}

	//TODO FIRST:: CHECK THIS
	if (result == 0)
	{
		////ReleaseSemaphore(handle, 1, 0);
		return 0;
	}

	////ReleaseSemaphore(handle, 1, 0);
	return result;
}

//TODO maybe change - dont make copy of cache
ClusterNo FileSystemPartition::getRootClusterNo(EntryNum entryNum)
{
	int numOfEntriesInSingleClusterHehehehehehehe = ClusterSize / ENTRYSIZE;
	//int idx = entryNum % numOfEntriesInSingleClusterHehehehehehehe == 0 ? entryNum / numOfEntriesInSingleClusterHehehehehehehe - 1 : entryNum / numOfEntriesInSingleClusterHehehehehehehe;
	int idx = entryNum / numOfEntriesInSingleClusterHehehehehehehe;
	int offset = idx;

	//you are making copy here

	ClusterNo writingCluster = rootIndex;
	ClusterNo result;
	
	if (idx >= ClusterSize / 2)
	{
		idx -= ClusterSize / 2;
		offset = idx % ClusterSize;
		//idx = idx % ClusterSize != 0 ? idx / ClusterSize : idx / ClusterSize - 1;
		idx = idx / ClusterSize;
		//TODO allocating and writing should be synchronus
		cache->read(rootIndex, idx + ClusterSize / 2, result); 

		if (result == 0)
		{
			writingCluster = allocateIndex(rootIndex, sizeof(ClusterNo)*(idx + ClusterSize / 2));
		}

		cache->read(result, sizeof(ClusterNo)*offset, result);
	}
	else
	{
		cache->read(rootIndex, sizeof(ClusterNo)*idx, result);
	}

	//TODO FIRST:: CHECK THIS
	if (result == 0)
	{
		result = allocateIndex(writingCluster, sizeof(ClusterNo)*offset);
	}

	return result;
}

//no need for locking because allocating should happen only when writer enteres!
ClusterNo FileSystemPartition::getPageNum(ClusterNo index, BytesCnt bytes)
{
	//int idx = bytes % ClusterSize != 0 ? bytes / ClusterSize : bytes / ClusterSize - 1;
	int idx = bytes / ClusterSize;
	int numEntries = ClusterSize / sizeof(ClusterNo);

	if (bytes == 0)
	{
		idx = 0;
	}

	ClusterNo result;
	ClusterNo dataCluster;
	int offset = idx;

	if (idx >= numEntries / 2)
	{
		idx -= numEntries / 2;
		//idx = idx % ClusterSize != 0 ? idx / ClusterSize : idx / ClusterSize - 1;
		idx = idx / numEntries;

		cache->read(index,(numEntries/ 2 + idx)*sizeof(ClusterNo), result);
		
		if (result == 0)
		{
			result = allocateIndex(index, (numEntries / 2 + idx)*sizeof(ClusterNo));
		}

		offset = idx % numEntries;

		cache->read(result, offset*sizeof(ClusterNo), dataCluster);
	}
	else
	{
		cache->read(index, offset*sizeof(ClusterNo), dataCluster);
		result = index;
		if (dataCluster == 4261281277)
		{
			int x = 2;
		}
	}

	if (dataCluster == 0)
	{
		dataCluster = allocateDataCluster(result, offset*sizeof(ClusterNo));
	}

	if (dataCluster == 4261281277)
	{
		int x=2;
	}

	return dataCluster;
}

ClusterNo FileSystemPartition::getClusterNoFromEntryNo(EntryNum entryNum)
{
	ClusterNo result = rootIndex;
	ClusterNo n = entryNum / 102;
	

	if (n >= ClusterSize / 2 / sizeof(ClusterNo) * 102)
	{
		int numOfEntriesInSecondIndex = ClusterSize / sizeof(ClusterNo) * 102;
		n -= ClusterSize / 2 / sizeof(ClusterNo) * 102;
		ClusterNo off = n / numOfEntriesInSecondIndex;
		int offset = n % numOfEntriesInSecondIndex;
		cache->read(rootIndex, ClusterSize / 2 + off*sizeof(ClusterNo), result);
		cache->read(result, offset*sizeof(ClusterNo), result);
	}
	else
	{
		cache->read(result, n*sizeof(ClusterNo), result);
	}

	return result;
}

void FileSystemPartition::writeSize(EntryNum n, int size)
{
	int entry = n % 102;
	ClusterNo clusterNo = getClusterNoFromEntryNo(n);
	char* sz = (char*)&size;
	cache->write(clusterNo, 4, entry*sizeof(Entry) + 16, sz);
}

void FileSystemPartition::deleteEntry(EntryNum n)
{
	int entry = n % 102;
	ClusterNo clusterNo = getClusterNoFromEntryNo(n);
	char buff[sizeof(Entry)];
	
	for (int i = 0; i < sizeof(Entry); i++)
	{
		buff[i] = 0;
	}

	cache->write(clusterNo, sizeof(Entry), entry*sizeof(Entry), buff);
}

ClusterNo FileSystemPartition::allocateDataCluster(ClusterNo clusterNo, int offset)
{
	ClusterNo toAdd = setFirstFree();
	cache->write(clusterNo, offset, toAdd);

	return toAdd;
}

ClusterNo FileSystemPartition::getNumOfClusters() const
{
	return _partition->getNumOfClusters();
}

ClusterNo FileSystemPartition::setFirstFree()
{
	return _bitvector->getFirstFree();
}