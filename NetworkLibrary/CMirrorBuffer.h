#pragma once
#include "CRingBuffer.h"
#include <iostream>
using namespace std;
class CMirrorBuffer
{
protected:
	CRingBuffer* _pBuf;
	int remainSize;
public:
	CMirrorBuffer(CRingBuffer* pBuf, int size): _pBuf(pBuf), remainSize(size) {};
	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	CMirrorBuffer& operator >> (T& data) throw(int)
	{
		if (remainSize < sizeof(T))
		{
			cout << "CMirrorBuffer remainSize Error\n";
			throw(remainSize);
		}
		int dequeueSize= _pBuf->Dequeue((char*)&data, sizeof(T));
		if (dequeueSize != sizeof(T))
		{
			cout << "CMirrorBuffer dequeue Error\n";
			throw(dequeueSize);
		}
		return *this;
	}
};