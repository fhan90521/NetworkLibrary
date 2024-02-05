#pragma once
#include "CRingBuffer.h"
#include "NetworkHeader.h"
#include "MyStlContainer.h"
#include <iostream>
using namespace std;
class CRecvBuffer
{
protected:
	CRingBuffer* _pBuf;
	int _remainSize;
	CRecvBuffer(const CRecvBuffer& src) = delete;
	CRecvBuffer& operator = (CRecvBuffer& src) = delete;
public:
	CRecvBuffer(CRingBuffer* pBuf, int size): _pBuf(pBuf), _remainSize(size) {};

	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	CRecvBuffer& operator >> (T& data) throw(int)
	{
		if (_remainSize < sizeof(T))
		{
			cout << "CRecvBuffer remainSize Error\n";
			throw(_remainSize);
		}
		int dequeueSize= _pBuf->Dequeue((char*)&data, sizeof(T));
		if (dequeueSize != sizeof(T))
		{
			cout << "CRecvBuffer dequeue Error\n";
			throw(dequeueSize);
		}
		_remainSize -= dequeueSize;
		return *this;
	}

	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	CRecvBuffer& operator >> (Vector<T>& vec) throw(int)
	{
		USHORT vecSize;
		int dequeueSize;

		if (_remainSize < sizeof(vecSize))
		{
			cout << "CRecvBuffer remainSize Error\n";
			throw(_remainSize);
		}

		dequeueSize = _pBuf->Dequeue((char*)&vecSize, sizeof(vecSize));
		if (dequeueSize != sizeof(vecSize))
		{
			cout << "CRecvBuffer dequeue Error\n";
			throw(dequeueSize);
		}
		_remainSize -= dequeueSize;
		
		if (vecSize > vec.size())
		{
			vec.resize(vecSize);
		}
		dequeueSize=_pBuf->Dequeue((char*)vec.data(), vecSize * sizeof(T));
		if (dequeueSize != vecSize * sizeof(T))
		{
			cout << "CRecvBuffer dequeue Error\n";
			throw(dequeueSize);
		}
		_remainSize -= dequeueSize;

		return *this;
	}

	template <typename T, typename size_t Size, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	CRecvBuffer& operator >> (Array<T,Size>& arr) throw(int)
	{
		int dequeueSize;
		dequeueSize = _pBuf->Dequeue((char*)arr.data(), Size * sizeof(T));
		if (dequeueSize != Size * sizeof(T))
		{
			cout << "CRecvBuffer dequeue Error\n";
			throw(dequeueSize);
		}
		_remainSize -= dequeueSize;
		return *this;
	}
	
	int GetData(char* chpDest, int iSize)
	{
		if (_remainSize < sizeof(iSize))
		{
			cout << "CRecvBuffer remainSize Error in GetData\n";
		}
		int dequeueSize = _pBuf->Dequeue(chpDest, iSize);
		_remainSize -= dequeueSize;
		return iSize;
	}

	bool Decode(NetHeader* pNetHeader)
	{
		/*BYTE prevE = pNetHeader->checkSum;
		BYTE prevP = pNetHeader->DecodeCheckSum();
		BYTE P;
		BYTE checkSum = 0;
		int index = _pBuf->GetFrontIndex();
		char* buf = _pBuf->GetBufferPtr();
		for (int i = 0; i < pNetHeader->len; i++)
		{
			P = (buf[index]) ^ (prevE + (pNetHeader->constKey) + i + 2);
			prevE = buf[index];
			buf[index] = P ^ (prevP + (pNetHeader->randKey) + i + 2);
			prevP = P;
			checkSum += buf[index];
			index = (index + 1) % (_pBuf->GetBufferSize() + 1);
		}
		if (checkSum != pNetHeader->checkSum)
		{
			return false;
		}*/
		return true;
	}
	
};