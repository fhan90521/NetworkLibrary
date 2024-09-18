#pragma once
#pragma warning( disable : 4290 )
#include "OS/MyWindow.h"
#include "Network/NetworkHeader.h"
#include "Memory/TlsObjectPool.h"
#include "Container/MyStlContainer.h"

#include <atomic>
#include <memory.h>
#include <type_traits>
#define CHECK_SENDBUF_CNT
class CSendBuffer
{
private:
	friend class IOCPServer;
	friend class IOCPClient;
	friend class IOCPDummyClient;
	enum enCSendBuffer: int
	{
		eBUFFER_DEFAULT = 1024, // 패킷의 기본 버퍼 사이즈.
		eBUFFER_MAX_SIZE = 4096
	};
	typedef TlsObjectPool<CSendBuffer, false> BufferPool;
#ifdef CHECK_SENDBUF_CNT
	inline static std::atomic<int> _allocCnt=0;
#endif

	friend class BufferPool;
	char* _buf;
	int	_bufferSize = eBUFFER_DEFAULT;
	int _front = sizeof(WanHeader);
	int _back = sizeof(WanHeader);
	LONG _refCnt = 0;
	bool _bSetHeader = false;
	static BufferPool _bufferPool;
	CSendBuffer()
	{
		_buf = new char[eBUFFER_DEFAULT];
	}
	virtual	~CSendBuffer()
	{
		delete[] _buf;
	}
	bool Resize(int iSize);
	void Encode()
	{
		WanHeader* pWanHeader = (WanHeader*)_buf;
		pWanHeader->randKey = rand();
		BYTE* payLoad = (BYTE*)&_buf[_front];
		BYTE P = pWanHeader->EncodeCheckSum();
		P = payLoad[0] ^ (P + (pWanHeader->randKey) + 2);
		payLoad[0] = P ^ (pWanHeader->checkSum + (pWanHeader->constKey) + 2);
		for (int i = 1; i < pWanHeader->len; i++)
		{
			P = payLoad[i] ^ (P + (pWanHeader->randKey) + i + 2);
			payLoad[i] = P ^ (payLoad[i - 1] + (pWanHeader->constKey) + i + 2);
		}
	}
	void SetLanHeader()
	{
		if (_bSetHeader == true)
		{
			return;
		}
		_bSetHeader = true;
		LanHeader* pLanHeader = GetLanHeader();
		pLanHeader->len = GetPayLoadSize();
	}
	void SetWanHeader()
	{
		if (_bSetHeader == true)
		{
			return;
		}
		_bSetHeader = true;
		WanHeader* pWanHeader = (WanHeader*)_buf;
		BYTE checkSum = 0;
		pWanHeader->code = WanHeader::NetCode;
		pWanHeader->len = GetPayLoadSize();
		BYTE* payLoad = (BYTE*)&_buf[_front];
		for (int i = 0; i < pWanHeader->len; i++)
		{
			checkSum += payLoad[i];
		}
		pWanHeader->checkSum = checkSum;
		Encode();
	}
	WanHeader* GetWanHeader()
	{
		return (WanHeader*)_buf;
	}
	LanHeader* GetLanHeader()
	{
		return (LanHeader*)(_buf + sizeof(WanHeader) - sizeof(LanHeader));
	}
public:
#ifdef CHECK_SENDBUF_CNT
	static LONG GetAllocCnt()
	{
		return _allocCnt;
	}
#endif
	static CSendBuffer* Alloc()
	{
		CSendBuffer* pBuf = _bufferPool.Alloc();
		pBuf->Clear();
#ifdef CHECK_SENDBUF_CNT
		_allocCnt++;
#endif
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
#ifdef CHECK_SENDBUF_CNT
			_allocCnt--;
#endif
		}
	}
	void Clear()
	{
		_front = sizeof(WanHeader);
		_back = sizeof(WanHeader);
		_bSetHeader = false;
	}

	int GetBufferSize() { return _bufferSize; }
	int GetPayLoadSize() { return _back - _front; }
	int GetFreeSize() { return _bufferSize - _back; }
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
	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> || std::is_same_v<T, wchar_t>>>
	CSendBuffer& operator << (T data) 
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

	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> || std::is_same_v<T, wchar_t>>>
	CSendBuffer& operator << (const Vector<T>& vec) 
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

	template <typename T, typename size_t Size, typename = std::enable_if_t<std::is_arithmetic_v<T> || std::is_same_v<T,wchar_t>>>
	CSendBuffer& operator << (const Array<T, Size>& arr) 
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

	template <typename T, typename = std::enable_if_t<std::is_same_v<T, std::string> || std::is_same_v<T, String>
													  ||std::is_same_v<T, std::wstring> || std::is_same_v<T, WString>>>
	CSendBuffer& operator << (const T& str)
	{
		using namespace std;
		USHORT strLen = str.size();
		if (GetFreeSize() < sizeof(strLen) + strLen*sizeof(T::value_type))
		{
			if (Resize(sizeof(strLen) + strLen * sizeof(T::value_type)) == false)
			{
				throw(GetPayLoadSize());
			}
		}
		*((USHORT*)&_buf[_back]) = strLen;
		_back += sizeof(strLen);

		memcpy(&_buf[_back], str.data(), strLen * sizeof(T::value_type));
		_back += strLen * sizeof(T::value_type);
		return *this;
	}
};