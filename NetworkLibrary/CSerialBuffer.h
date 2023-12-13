#pragma once
#pragma warning( disable : 4290 )
#include <memory.h>
#include <Windows.h>
#include <type_traits>
#include "NetworkHeader.h"
using namespace std;
class CSerialBuffer
{
protected:
	LONG _refCnt = 0;
	char* _buf;
	int	_bufferSize;
	int _front = sizeof(NetHeader);
	int _back = sizeof(NetHeader);
	
public:

	enum en_CSerialBuffer
	{
		eBUFFER_DEFAULT = 1400, // 패킷의 기본 버퍼 사이즈.
		eBUFFER_MAX_SIZE = 5000
	};

	CSerialBuffer(int iBufferSize = eBUFFER_DEFAULT) :_bufferSize(iBufferSize)
	{
		_buf = new char[iBufferSize];
	}

	virtual	~CSerialBuffer()
	{
		delete[] _buf;
	}

	void Clear()
	{
		_front = 0;
		_back = 0;
	}

	void IncrementRefCnt()
	{
		InterlockedIncrement(&_refCnt);
	}
	void DecrementRefCnt()
	{
		if (InterlockedDecrement(&_refCnt) == 0)
		{
			delete this;
		}
	}

	int	GetBufferSize() { return _bufferSize; }
	int	GetUseSize() { return _back - _front; }
	int GetFreeSize() { return _bufferSize - _back; }
	char* GetReadPtr() { return &_buf[_front]; }
	char* GetWritePtr() { return &_buf[_back]; }
	
	NetHeader* GetWanHeaderPtr() { return (NetHeader*)_buf;}
	NetHeader* GetLanHeaderPtr() { return (NetHeader*)(&_buf[DIFF_HEADER_SIZE]);}

	bool Resize(int iSize);

	int	MoveFront(int iSize);
	int	MoveBack(int iSize);

	int	GetData(char* chpDest, int iSize);
	int	PutData(char* chpSrc, int iSize);

	CSerialBuffer(const CSerialBuffer& src) = delete;
	CSerialBuffer& operator = (CSerialBuffer& src) = delete;

	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	CSerialBuffer& operator << (T data) throw(int)
	{
		if (GetFreeSize() < sizeof(T))
		{
			if (Resize(sizeof(T)) == false)
			{
				throw(GetUseSize());
			}
		}
		*((T*)&_buf[_back]) = data;
		_back += sizeof(T);
		return *this;
	}

	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	CSerialBuffer& operator >> (T& data) throw(int)
	{
		if (GetUseSize() < sizeof(T))
		{
			throw(GetUseSize());
		}
		data = *((T*)&_buf[_front]);
		_front += sizeof(T);
		return *this;
	}
};