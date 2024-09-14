#include "CRingBuffer.h"
#include <memory.h>
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
bool CRingBuffer::ReSize(int iSize)
{
	int new_totalSize = MAX(_totalSize + iSize, _totalSize + _totalSize / 2);
	if (new_totalSize > MAX_RINGBUFFER_SIZE)
	{
		return false;
	}
	int useSize = GetUseSize();
	char* newBuf = new char[new_totalSize + 1];
	Dequeue(newBuf, useSize);
	delete _buf;
	_buf = newBuf;

	_totalSize = new_totalSize;
	_front = 0;
	_back = useSize;

	return true;
}
int CRingBuffer::Enqueue(char* data, int iSize)
{
	int freeSize = GetFreeSize();
	if (freeSize < iSize)
	{
		iSize = freeSize;
	}

	int directEnqueSize = DirectEnqueueSize();
	if (iSize <= directEnqueSize)
	{
		memcpy(GetBackBufferPtr(), data, iSize);
	}
	else
	{
		memcpy(GetBackBufferPtr(), data, directEnqueSize);
		memcpy(&_buf[0], data + directEnqueSize, iSize - directEnqueSize);
	}

	_back = (_back+iSize)%(_totalSize + 1);
	return iSize;
}
int CRingBuffer::Dequeue(char* dest, int iSize)
{
	int useSize = GetUseSize();
	if (useSize < iSize)
	{
		iSize = useSize;
	}
	int directDequeueSize = DirectDequeueSize();
	if (iSize <= directDequeueSize)
	{
		memcpy(dest, GetFrontBufferPtr(), iSize);
	}
	else
	{
		memcpy(dest, GetFrontBufferPtr(), directDequeueSize);
		memcpy(dest + directDequeueSize, &_buf[0], iSize - directDequeueSize);

	}

	_front = (_front+iSize)% (_totalSize + 1);
	return iSize;
}

int CRingBuffer::Peek(char* dest, int iSize)
{
	int useSize = GetUseSize();
	if (useSize < iSize)
	{
		iSize = useSize;
	}
	int directDequeueSize = DirectDequeueSize();
	if (iSize <= directDequeueSize)
	{
		memcpy(dest, GetFrontBufferPtr(), iSize);
	}
	else
	{
		memcpy(dest, GetFrontBufferPtr(), directDequeueSize);
		memcpy(dest + directDequeueSize, &_buf[0], iSize - directDequeueSize);
	}

	return iSize;
}
int CRingBuffer::MoveBack(int iSize)
{
	int freeSize = GetFreeSize();
	if (freeSize < iSize)
	{
		iSize = freeSize;
	}
	_back = (_back + iSize) % (_totalSize + 1);
	return iSize;
}
int CRingBuffer::MoveFront(int iSize)
{
	int useSize = GetUseSize();
	if (useSize < iSize)
	{
		iSize = useSize;
	}
	_front = (_front + iSize) % (_totalSize + 1);
	return iSize;
}

