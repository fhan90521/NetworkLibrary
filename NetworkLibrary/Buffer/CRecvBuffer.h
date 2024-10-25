#pragma once
#include "CRingBuffer.h"
#include "Network/NetworkHeader.h"
#include "Container/MyStlContainer.h"
#include "DebugTool/Log.h"
#include <type_traits>
class CRecvBuffer
{
private:
	CRingBuffer* _pBuf;
	int _remainSize;// 초기값은 네트워크 헤더 Len 링버퍼의 FreeSize랑 구분되어야 함
	CRecvBuffer(const CRecvBuffer& src) = delete;
	CRecvBuffer& operator = (CRecvBuffer& src) = delete;
public:
	CRecvBuffer(CRingBuffer* pBuf, int size): _pBuf(pBuf), _remainSize(size) {};

	template <typename T, std::enable_if_t<std::is_arithmetic_v<T> || std::is_same_v<T, wchar_t>,int > uniquifier = 0>
	CRecvBuffer& operator >> (T& data) 
	{
		if (_remainSize < sizeof(T))
		{
			//cout << "CRecvBuffer remainSize Error\n";
			throw(_remainSize);
		}
		int dequeueSize= _pBuf->Dequeue((char*)&data, sizeof(T));
		if (dequeueSize != sizeof(T))
		{
			Log::LogOnFile(Log::DEBUG_LEVEL, "CRecvBuffer dequeue Error\n");
			throw(dequeueSize);
		}
		_remainSize -= dequeueSize;
		return *this;
	}

	

	template <typename T, typename size_t Size, typename = std::enable_if_t<std::is_arithmetic_v<T> || std::is_same_v<T, wchar_t>>>
	CRecvBuffer& operator >> (Array<T,Size>& arr) 
	{
		if (_remainSize < Size * sizeof(T))
		{
			Log::LogOnFile(Log::DEBUG_LEVEL, "CRecvBuffer remainSize Error\n");
			throw(_remainSize);
		}
		int dequeueSize;
		dequeueSize = _pBuf->Dequeue((char*)arr.data(), Size * sizeof(T));
		if (dequeueSize != Size * sizeof(T))
		{
			Log::LogOnFile(Log::DEBUG_LEVEL, "CRecvBuffer dequeue Error\n");
			throw(dequeueSize);
		}
		_remainSize -= dequeueSize;
		return *this;
	}
	
	int GetData(char* chpDest, int iSize)
	{
		if (_remainSize < sizeof(iSize))
		{
			Log::LogOnFile(Log::SYSTEM_LEVEL, "CRecvBuffer remainSize Error\n");
		}
		int dequeueSize = _pBuf->Dequeue(chpDest, iSize);
		_remainSize -= dequeueSize;
		return iSize;
	}

	bool Decode(WanHeader* pWanHeader)
	{
		BYTE prevE = pWanHeader->checkSum;
		BYTE prevP = pWanHeader->DecodeCheckSum();
		BYTE P;
		BYTE checkSum = 0;
		int index = _pBuf->GetFrontIndex();
		char* buf = _pBuf->GetBufferPtr();
		for (int i = 0; i < pWanHeader->len; i++)
		{
			P = (buf[index]) ^ (prevE + (pWanHeader->constKey) + i + 2);
			prevE = buf[index];
			buf[index] = P ^ (prevP + (pWanHeader->randKey) + i + 2);
			prevP = P;
			checkSum += buf[index];
			index = (index + 1) % (_pBuf->GetBufferSize() + 1);
		}
		if (checkSum != pWanHeader->checkSum)
		{
			return false;
		}
		return true;
	}

	template <typename T, std::enable_if_t<std::is_same_v<T, std::string> || std::is_same_v<T, String>
									    ||std::is_same_v<T, std::wstring> || std::is_same_v<T, WString>
	|| (std::is_same_v<T, Vector<typename T::value_type>>
		&& (std::is_arithmetic_v<typename T::value_type> || std::is_same_v<typename T::value_type, wchar_t>)),int> uniquifier = 0>
	CRecvBuffer& operator >> (T& container)
	{
		using namespace std;
		USHORT containerSize;
		int dequeueSize;


		if (_remainSize < sizeof(containerSize))
		{
			Log::LogOnFile(Log::DEBUG_LEVEL, "CRecvBuffer remainSize Error\n");
			throw(_remainSize);
		}

		dequeueSize = _pBuf->Dequeue((char*)&containerSize, sizeof(containerSize));
		if (dequeueSize != sizeof(containerSize))
		{
			Log::LogOnFile(Log::DEBUG_LEVEL, "CRecvBuffer dequeue Error\n");
			throw(dequeueSize);
		}
		_remainSize -= dequeueSize;

		if (_remainSize < containerSize *sizeof(T::value_type))
		{
			Log::LogOnFile(Log::DEBUG_LEVEL, "CRecvBuffer remainSize Error\n");
			throw(_remainSize);
		}

		if (containerSize > container.size())
		{
			container.resize(containerSize);
		}


		dequeueSize = _pBuf->Dequeue((char*)container.data(), containerSize * sizeof(T::value_type));
		if (dequeueSize != containerSize * sizeof(T::value_type))
		{
			Log::LogOnFile(Log::DEBUG_LEVEL, "CRecvBuffer dequeue Error\n");
			throw(dequeueSize);
		}
		_remainSize -= dequeueSize;

		return *this;
	}
};