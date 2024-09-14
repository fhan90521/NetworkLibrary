#include "CSendBuffer.h"
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
CSendBuffer::BufferPool CSendBuffer::_bufferPool;
bool CSendBuffer::Resize(int iSize)
{
	int new_bufferSize = MAX(_bufferSize+iSize,_bufferSize + _bufferSize / 2);
	if (new_bufferSize > eBUFFER_MAX_SIZE)
	{
		return false;
	}
	char* newBuf = new char[new_bufferSize];
	memcpy(newBuf, &_buf[0], _back);

	delete _buf;
	_buf = newBuf;
	
	_bufferSize = new_bufferSize;
	return true;
}

