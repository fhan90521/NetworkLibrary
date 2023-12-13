#include "CSerialBuffer.h"
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
bool CSerialBuffer::Resize(int iSize)
{
	int new_bufferSize = MAX(_bufferSize+iSize,_bufferSize + _bufferSize / 2);
	if (new_bufferSize > eBUFFER_MAX_SIZE)
	{
		return false;
	}
	char* newBuf = new char[new_bufferSize];
	memcpy(newBuf, &_buf[_front], GetUseSize());

	delete _buf;
	_buf = newBuf;
	
	_bufferSize = new_bufferSize;
	_back = GetUseSize();
	_front = 0;


	return true;
}

int	CSerialBuffer::MoveFront(int iSize)
{
	if (iSize > GetUseSize())
	{
		iSize = GetUseSize();
	}
	_front += iSize;
	return iSize;
}

int	CSerialBuffer::MoveBack(int iSize)
{
	if (iSize > GetFreeSize())
	{
		iSize = GetFreeSize();
	}
	_back += iSize;
	return iSize;
}

int	CSerialBuffer::GetData(char* chpDest, int iSize)
{
	if (GetUseSize() < iSize)
	{
		iSize = GetUseSize();
	}
	memcpy(chpDest, &_buf[_front], iSize);
	_front += iSize;
	return iSize;
}

int	CSerialBuffer::PutData(char* chpSrc, int iSize)
{
	if (GetFreeSize() < iSize)
	{
		iSize = GetFreeSize();
	}
	memcpy(&_buf[_back], chpSrc, iSize);
	_back += iSize;
	return iSize;
}

