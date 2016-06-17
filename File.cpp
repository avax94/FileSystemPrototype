#include "File.h"
#include "KernelFile.h"

File::File()
{
}


File::~File()
{
	delete myImpl;
}

char File::write(BytesCnt bcnt, char * buffer)
{
	return myImpl->write(bcnt, buffer);
}

BytesCnt File::read(BytesCnt bcnt, char * buffer)
{
	return myImpl->read(bcnt, buffer);
}

char File::seek(BytesCnt bcnt)
{
	return myImpl->seek(bcnt);
}

BytesCnt File::filePos()
{
	return myImpl->filePos();
}

char File::eof()
{
	return myImpl->eof();
}

BytesCnt File::getFileSize()
{
	return myImpl->getFileSize();
}

char File::truncate()
{
	return myImpl->truncate();
}
