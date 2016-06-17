#pragma once
#include "part.h"
#include "fs.h"
class FileSystemPartition;
///TODO this implementation is bad... you are loading from cache, but data might not be there when you look for it ? make copy	
class IndexParser
{
	void parse(const char*, ClusterNo*);
public:
	static ClusterNo getPageNum(FileSystemPartition* p, ClusterNo index, BytesCnt bytes);
};

