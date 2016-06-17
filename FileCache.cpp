#include "FileCache.h"
#include "FileSystemPartition.h"
#include "bitvector.h"

void FileCache::update(ClusterNo clusterNo)
{
	if (clusterNo != this->clusterNo)
	{
		if (dirty)
		{
			while (partition->writeCluster(this->clusterNo, dataCluster)==0);
			dirty = false;
		}

		while (partition->readCluster(clusterNo, dataCluster)==0);
	
		this->clusterNo = clusterNo;
	}
}

void FileCache::swipe(bool fIndex, int offset, FileSystemPartition* p)
{
	bitvector* b = p->getBitvector();
	ClusterNo data;

	if (fIndex)
	{
		for (int i = offset; i < ClusterSize; i+=sizeof(ClusterNo))
		{
			read(true, i, data);
			
			if (data == 0)
			{
				break;
			}

			updateSecondIndex(data);
			swipe(false, 0, p);
			b->reset(data);
			write(true, offset, 0);
		}
	}
	else
	{
		for (int i = offset; i < ClusterSize; i++)
		{
			read(false, i, data);
			b->reset(data);
			write(true, offset, 0);
		}
	}
}

void FileCache::updateSecondIndex(ClusterNo clusterNo)
{
	if (clusterNo != sIndex)
	{
		if (sdirty)
		{
			while (partition->writeCluster(sIndex, secondIndex) == 0);
			sdirty = false;
		}

		while (partition->readCluster(clusterNo, secondIndex) == 0);

		sIndex = clusterNo;
	}
}

BytesCnt FileCache::read(ClusterNo clusterNo, BytesCnt bCnt, int offset, char * result)
{
	BytesCnt i;
	update(clusterNo);

	for (i = 0; i < bCnt && offset + i < ClusterSize; i++)
	{
		result[i] = dataCluster[offset + i];
	}

	return i;
}

void FileCache::write(ClusterNo clusterNo, BytesCnt bCnt, int offset, char * result)
{
	update(clusterNo);

	for (int i = 0; i < bCnt && offset + i < ClusterSize; i++)
	{
		dataCluster[offset + i] = result[i];
	}

	dirty = true;
}

void FileCache::write(bool fIndex, BytesCnt offset, ClusterNo data)
{
	char* d = (char*)&data;

	if (!fIndex)
	{
		sdirty = true;

		for (int i = 0; i < sizeof(ClusterNo); i++)
		{
			secondIndex[offset + i] = d[i];
		}
	}
	else
	{
		fdirty = true;

		for (int i = 0; i < sizeof(ClusterNo); i++)
		{
			firstIndex[offset + i] = d[i];
		}
	}
}

void FileCache::read(bool fIndex, BytesCnt offset, ClusterNo & data)
{
	char* d = (char*)&data;

	if (!fIndex)
	{
		for (int i = 0; i < sizeof(ClusterNo); i++)
		{
			d[i] = secondIndex[offset + i];
		}
	}
	else
	{
		for (int i = 0; i < sizeof(ClusterNo); i++)
		{
			d[i] = firstIndex[offset + i];
		}
	}
}

FileCache::FileCache(Partition* p, ClusterNo fIdx)
{
	fIndex = fIdx;
	sIndex = 0;
	partition = p;
	dataCluster = new char[ClusterSize];
	firstIndex = new char[ClusterSize];
	secondIndex = new char[ClusterSize];
	dirty = false;
	fdirty = false;
	sdirty = false;

	while (partition->readCluster(fIndex, firstIndex) == 0);
}

void FileCache::setZeroClusterSecondIndex()
{
	for (int i = 0; i < ClusterSize; i++)
	{
		secondIndex[i] = 0;
	}
}


FileCache::~FileCache()
{
	if (dirty)
	{
		while (partition->writeCluster(clusterNo, dataCluster) == 0);
	}
	if (fdirty)
	{
		while (partition->writeCluster(fIndex, firstIndex) == 0);
	}
	if (sdirty)
	{
		while (partition->writeCluster(sIndex, secondIndex) == 0);
	}

	delete[]dataCluster;
	delete[]firstIndex;
	delete[]secondIndex;
}
