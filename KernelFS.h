#pragma once

#include "fs.h"
#include "part.h"
#include <vector>
#include <windows.h>
#include <unordered_map>
#include <string>
#define NumPartitions 26

using namespace std;

class FileSystemPartition;
class bitvector;
class KernelFile;
class FileHandle;

class KernelFS {
	Entry parseEntry(char* buff);
	EntryNum allocateEntry(FileSystemPartition*,Entry);
	void writeEntry(int offset, Entry entry, ClusterNo clusterNo, FileSystemPartition * p);
	SRWLOCK* srw;
	HANDLE handle;
	unordered_map<string, Entry> files;
	static char* parseName(char * name, char part);
public:
	ClusterNo getClusterNoFromEntryNo(EntryNum entryNum);
	void loadFiles(FileSystemPartition*);
	~KernelFS();
	char mount(Partition* partition);
	char unmount(char part);
	char format(char part);
	char readRootDir(char part, EntryNum n, Directory &d);
	char writeSize(char part, int size, char* fname);
	char loadFiles(char part);
	FileHandle* doesExist(char* fname);
	KernelFile* open(char* fname, char mode);
	char deleteFile(char* fname);
	
	KernelFS();
};

void initialize();
void finilize();