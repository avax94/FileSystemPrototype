#include "RootCache.h"



RootCache::RootCache()
{
}


RootCache::~RootCache()
{
}

void RootCache::write(Index index, ClusterNo clusterNo, BytesCnt offset, ClusterNo data)
{

}

void RootCache::read(Index index, ClusterNo clusterNo, BytesCnt offset, ClusterNo & data)
{
}

BytesCnt RootCache::read(Index index, ClusterNo clusterNo, BytesCnt bCnt, int offset, char * result)
{
	BytesCnt i;
	Cluster* clst;

	switch (index)
	{
	case ROOT:
		clst = &rootIndex;
		break;
	case SECOND:
		clst = &secondIndex;
		break;
	case DATA:
		clst = &data;
	}


	for (i = 0; i < bCnt && offset + i < ClusterSize; i++)
	{
	//	result[i] = buff[offset + i];
	}

	return i;
}

void RootCache::write(Index index, ClusterNo clusterNo, BytesCnt bCnt, int offset, char * result)
{
}

void Cluster::startRead()
{
	AcquireSRWLockShared(srwlock);
}

void Cluster::startWrite()
{
	AcquireSRWLockExclusive(srwlock);
}

void Cluster::endRead()
{
	ReleaseSRWLockShared(srwlock);
}

void Cluster::endWrite()
{
	ReleaseSRWLockExclusive(srwlock);
}

void Cluster::update(ClusterNo)
{
	//TODO implement
}

Cluster::Cluster()
{
	srwlock = new SRWLOCK(SRWLOCK_INIT);
}

Cluster::~Cluster()
{
	delete srwlock;
}
