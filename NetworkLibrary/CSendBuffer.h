#pragma once
#pragma warning( disable : 4290 )
#include <memory.h>
#include <type_traits>
#include "NetworkHeader.h"
#include "TlsObjectPool.h"
#include "MyWindow.h"
#include "MyStlContainer.h"
#include <iostream>
using namespace std;
class CSendBuffer
{
protected:
	friend class IOCPServer;
	enum enCSendBuffer
	{
		eBUFFER_DEFAULT = 512, // 패킷의 기본 버퍼 사이즈.
		eBUFFER_MAX_SIZE = 5000
	};
	typedef TlsObjectPool<CSendBuffer, false> BufferPool;
	friend class BufferPool;
	char* _buf;
	int	_bufferSize;
	int _front = sizeof(NetHeader);
	int _back = sizeof(NetHeader);
	LONG _refCnt = 0;
	static BufferPool _bufferPool;
	CSendBuffer(int iBufferSize = eBUFFER_DEFAULT) :_bufferSize(iBufferSize)
	{
		_buf = new char[iBufferSize];
	}
	virtual	~CSendBuffer()
	{
		delete[] _buf;
	}
	bool Resize(int iSize);
public:
	static LONG GetAllocCnt()
	{
		return _bufferPool.GetAllocCnt();
	}
	NetHeader* GetNetHeader()
	{
		return (NetHeader*)_buf;
	}
	void SetLanHeader()
	{
		NetHeader* pNetHeader = (NetHeader*)_buf;
		pNetHeader->len = GetPayLoadSize();
	}
	void SetWanHeader()
	{
		/*NetHeader* pNetHeader = (NetHeader*)_buf;
		BYTE checkSum = 0;
		pNetHeader->code = NetHeader::NetCode;
		pNetHeader->len = GetPayLoadSize();
		BYTE* payLoad = (BYTE*)GetReadPtr();
		for (int i = 0; i < pNetHeader->len; i++)
		{
			checkSum += payLoad[i];
		}
		pNetHeader->checkSum = checkSum;*/
	}
	void Encode()
	{
		/*NetHeader* pNetHeader = (NetHeader*)_buf;
		pNetHeader->randKey = rand();
		BYTE* payLoad = (BYTE*)GetReadPtr();
		BYTE P = pNetHeader->EncodeCheckSum();
		P = payLoad[0] ^ (P + (pNetHeader->randKey) + 2);
		payLoad[0] = P ^ (pNetHeader->checkSum + (pNetHeader->constKey) + 2);
		for (int i = 1; i < pNetHeader->len; i++)
		{
			P = payLoad[i] ^ (P + (pNetHeader->randKey) + i + 2);
			payLoad[i] = P ^ (payLoad[i - 1] + (pNetHeader->constKey) + i + 2);
		}*/
	}

	static CSendBuffer* Alloc()
	{
		CSendBuffer* pBuf = _bufferPool.Alloc();
		pBuf->Clear();
		return pBuf;
	}
	void IncrementRefCnt()
	{
		InterlockedIncrement(&_refCnt);
	}
	void DecrementRefCnt()
	{
		if (InterlockedDecrement(&_refCnt) == 0)
		{
			_bufferPool.Free(this);
		}
	}
	void Clear()
	{
		_front = sizeof(NetHeader);
		_back = sizeof(NetHeader);
	}

	int GetBufferSize() { return _bufferSize; }
	int GetPayLoadSize() { return _back - _front; }
	int GetPacketSize() { return _back; }
	int GetFreeSize() { return _bufferSize - _back; }
	char* GetReadPtr() { return &_buf[_front]; }
	char* GetWritePtr() { return &_buf[_back]; }

	CSendBuffer(const CSendBuffer& src) = delete;
	CSendBuffer& operator = (CSendBuffer& src) = delete;
	
	int	PutData(char* chpSrc, int iSize)
	{
		if (GetFreeSize() < iSize)
		{
			iSize = GetFreeSize();
		}
		memcpy(&_buf[_back], chpSrc, iSize);
		_back += iSize;
		return iSize;
	}
	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	CSendBuffer& operator << (T data) throw(int)
	{
		if (GetFreeSize() < sizeof(T))
		{
			if (Resize(sizeof(T)) == false)
			{
				throw(GetPayLoadSize());
			}
		}
		*((T*)&_buf[_back]) = data;
		_back += sizeof(T);
		return *this;
	}

	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	CSendBuffer& operator << (const Vector<T>& vec) throw(int)
	{
		USHORT vecSize = vec.size();
		if (GetFreeSize() < sizeof(vecSize) + vecSize*sizeof(T))
		{
			if (Resize(sizeof(vecSize) + vecSize * sizeof(T)) == false)
			{
				throw(GetPayLoadSize());
			}
		}
		*((USHORT*)&_buf[_back]) = vecSize;
		_back += sizeof(vecSize);

		memcpy(&_buf[_back], vec.data(), vecSize * sizeof(T));
		_back += vecSize * sizeof(T);
		return *this;
	}

	template <typename T, typename size_t Size, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	CSendBuffer& operator << (const Array<T, Size>& arr) throw(int)
	{
		USHORT arrLen= Size * sizeof(T);
		if (GetFreeSize() < arrLen)
		{
			if (Resize(arrLen) == false)
			{
				throw(GetPayLoadSize());
			}
		}
		memcpy(&_buf[_back], arr.data(), arrLen);
		_back += arrLen;
		return *this;
	}
};