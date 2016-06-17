#pragma once
#include <Windows.h>
#include <string>
#include <unordered_map>


using namespace std;

class Monitor
{
	HANDLE mutex;
	HANDLE sem;
	SRWLOCK* srwLock;
	char partitionName;
	bool formating;

	static unordered_map<string, HANDLE> mut;
	static unordered_map<string, SRWLOCK*> hash;
	unsigned int cnt;
	unsigned int rCnt = 0;

	static string charToString(char *fname);
public:
	Monitor(char part);
	~Monitor();

	bool open();
	void close();
	static void startRead(char*);
	static void endRead(char*);
	static void startWrite(char*);



	static void wMutEx(char*);
	static void sigMutEx(char*);
	

	void startReadRoot();
	void endReadRoot();
	void startFormating();
	void endFormating();
	
	static void endWrite(char* fname);
};

