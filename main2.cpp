#include "part.h"
#include "KernelFS.h"
#include "LRUCache.h"
#include "KernelFile.h"
#include <iostream>
#include "fs.h"
#include "File.h"
#include <fstream>

using namespace std;

int main2()
	{
	Partition *p = new Partition("p1.ini");
	initialize();
	KernelFS kfs;
	FS::mount(p);
	FS::format('a');
	File* f1 = FS::open("a:\\hehe.txt", 'w');
	FILE *f = fopen("ulaz3.jpg", "rb");
	if (f == 0) {
		cout << "GRESKA: Nije nadjen ulazni fajl 'ulaz.dat' u os domacinu!" << endl;
		system("PAUSE");
		return 0;//exit program
	}


	char * ulazBuffer = new char[32 * 1024 * 1024];//32MB
	int ulazSize = fread(ulazBuffer, 1, 32 * 1024 * 1024, f);

	ofstream fout("izlaz5.jpg", ios::out | ios::binary);

	
	f1->write(ulazSize, ulazBuffer);
	f1->seek(0);
	char *buff = new char[f1->getFileSize()];
	f1->read(f1->getFileSize() + 3, buff);
	fout.write(buff, f1->getFileSize());

	delete f;
	delete ulazBuffer;
	/*FS::format('a');
	File *kff = FS::open("a:\\nesto.est", 'w');

	char buff[2 * ClusterSize] = "Ovo je ulazni tekst fajl!!! T E S T   	!!!  *********************************   ** 			***				****           		xD";

	string s = "Ovo je ulazni tekst fajl!!! T E S T   	!!!  *********************************   ** 			***				****           		xD";
	kff->write(s.size(), buff);

	delete kff;

	FS::unmount('a');

	FS::mount(p);

	kff = FS::open("a:\\nesto.est", 'r');

	for (int i = 0; i < 2 * ClusterSize; i++)
	{
		buff[i] = 0;
	}

	kff->read(kff->getFileSize(), buff);

	for (int i = 0; i < kff->getFileSize(); i++)
	{
	cout << buff[i];
	}

	*/
	finilize();

	system("pause");
	return 1;
}
