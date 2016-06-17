#pragma once
#include "fs.h"
#include "part.h"
#include "FileHandle.h"
class Index;
class FileSystemPartition;
class FileCache;

class KernelFile
{
public:
	~KernelFile(); //zatvaranje fajla
	char write(BytesCnt, char* buffer);
	BytesCnt read(BytesCnt, char* buffer);
	ClusterNo getPageNum(BytesCnt bytes);
	ClusterNo allocateIndex(int offset);
	ClusterNo allocateDataCluster(bool findex, int offset);
	void deallocateClusters(ClusterNo startingCluster);
	char seek(BytesCnt);
	BytesCnt filePos();
	char eof();
	void updateCurrentCluster();
	BytesCnt getFileSize();
	char truncate();
private:
	FileCache * cache;
	bool dirty;
	bool sizeDirty;
	char name[128];
	bool toRead;
	FileHandle* fhandle;
	FileSystemPartition* partition;
	char write(char);
	char read(char&);
	BytesCnt cursor;
	BytesCnt clusterCursor;
	ClusterNo index;
	BytesCnt size;
	ClusterNo currentCluster;
	friend class FS;
	friend class KernelFS;
	KernelFile(FileSystemPartition*, ClusterNo index, bool r, char* fname, int size, FileHandle *f); //objekat fajla se može kreirati samo otvaranjem
};

