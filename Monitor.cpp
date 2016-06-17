#include "Monitor.h"
#include <windows.h>
#include <string>
#include <unordered_map>

using namespace std;

unordered_map<string, SRWLOCK*> Monitor::hash;
unordered_map<string, HANDLE> Monitor::mut;

Monitor::Monitor(char part)
{
	formating = false;
	srwLock = new SRWLOCK();
	InitializeSRWLock(srwLock);
	partitionName = part;
	string mutexName = "Monitor" + part;
	std::wstring stemp = std::wstring(mutexName.begin(), mutexName.end());
	LPCWSTR sw = stemp.c_str();
	mutex = CreateSemaphore(0, 1, 1, 0);
	cnt = 0;
	string semName = "Sem" + part;
	stemp = std::wstring(mutexName.begin(), mutexName.end());
	sw = stemp.c_str();
	sem = CreateSemaphore(0, 1, 1, 0);
}

void Monitor::close()
{
	WaitForSingleObject(mutex, INFINITE);
	cnt--;
	
	if (cnt == 0 && rCnt == 0)
	{
		ReleaseSemaphore(sem, 1, 0);
	}

	ReleaseSemaphore(mutex, 1, 0);
}

void Monitor::endReadRoot()
{
	WaitForSingleObject(mutex, INFINITE);
	rCnt--;

	if (cnt == 0 && rCnt == 0)
	{
		ReleaseSemaphore(sem, 1, 0);
	}

	ReleaseSemaphore(mutex, 1, 0);
}

void Monitor::startReadRoot()
{
	WaitForSingleObject(mutex, INFINITE);
	rCnt++;
	if (cnt == 0 && rCnt == 1)
	{
		WaitForSingleObject(sem, INFINITE);
	}
	ReleaseSemaphore(mutex, 1, 0);
}

bool Monitor::open()
{
	WaitForSingleObject(mutex, INFINITE);
	if (!formating)
	{
		cnt++;
		if (cnt == 1 && rCnt == 0)
		{
			WaitForSingleObject(sem, INFINITE);
		}
		ReleaseSemaphore(mutex, 1, 0);
		return true;
	}

	ReleaseSemaphore(mutex, 1, 0);
	return false;
}

string Monitor::charToString(char *fname)
{
	string s;
	int i = 0;
	while (fname[i] != '\0')
	{
		s += fname[i++];
	}

	s += '\0';

	return s;
}
void Monitor::startRead(char* fname)
{
	string sName = charToString(fname);
	if (hash.find(sName) == hash.end())
	{
		hash[sName] = new SRWLOCK(SRWLOCK_INIT);
	}
	
	AcquireSRWLockShared(hash[sName]);
}

void Monitor::endRead(char *fname)
{
	//todo: sync
	string sName = charToString(fname);

	ReleaseSRWLockShared(hash[sName]);
}

void Monitor::startWrite(char* fname)
{
	//todo: sync
	string sName = charToString(fname);
	if (hash.find(sName) == hash.end())
	{
		hash[sName] = new SRWLOCK(SRWLOCK_INIT);
		//InitializeSRWLock(hash[sName]);
	}

	AcquireSRWLockExclusive(hash[sName]);
}

void Monitor::wMutEx(char *fname)
{
	string s = charToString(fname);

	if (mut.find(s) == mut.end())
	{
		mut[s] = CreateSemaphore(0, 1, 1, 0);
	}

	WaitForSingleObject(mut[s], INFINITE);
}

void Monitor::sigMutEx(char *fname)
{
	string name = charToString(fname);
	ReleaseSemaphore(mut[name], 1, 0);
}

void Monitor::endWrite(char *fname)
{	
	string sName = charToString(fname);
	ReleaseSRWLockExclusive(hash[sName]);
}

void Monitor::startFormating()
{
	WaitForSingleObject(mutex, INFINITE);
	formating = true;
	ReleaseSemaphore(mutex, 1, 0);

	WaitForSingleObject(sem, INFINITE);
}

void Monitor::endFormating()
{
	WaitForSingleObject(mutex, INFINITE);
	formating = false;
	ReleaseSemaphore(mutex, 1, 0);

	ReleaseSemaphore(sem, 1, 0);
}

Monitor::~Monitor()
{
	CloseHandle(mutex);
	CloseHandle(sem);
	delete srwLock;
}


