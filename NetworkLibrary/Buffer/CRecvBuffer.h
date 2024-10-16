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
	int _remainSize;
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

	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> || std::is_same_v<T, wchar_t>>>
	CRecvBuffer& operator >> (Vector<T>& vec)
	{
		USHORT vecSize;
		int dequeueSize;

		
		if (_remainSize < sizeof(vecSize))
		{
			Log::LogOnFile(Log::DEBUG_LEVEL, "CRecvBuffer remainSize Error\n");
			throw(_remainSize);
		}

		dequeueSize = _pBuf->Dequeue((char*)&vecSize, sizeof(vecSize));
		if (dequeueSize != sizeof(vecSize))
		{
			Log::LogOnFile(Log::DEBUG_LEVEL, "CRecvBuffer dequeue Error\n");
			throw(dequeueSize);
		}
		_remainSize -= dequeueSize;
		
		if (_remainSize < vecSize*sizeof(T))
		{
			Log::LogOnFile(Log::DEBUG_LEVEL, "CRecvBuffer remainSize Error\n");
			throw(_remainSize);
		}

		if (vecSize > vec.size())
		{
			vec.resize(vecSize);
		}


		dequeueSize=_pBuf->Dequeue((char*)vec.data(), vecSize * sizeof(T));
		if (dequeueSize != vecSize * sizeof(T))
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
									    ||std::is_same_v<T, std::wstring> || std::is_same_v<T, WString>,int> uniquifier = 0>
	CRecvBuffer& operator >> (T& str)
	{
		using namespace std;
		USHORT strLen;
		int dequeueSize;


		if (_remainSize < sizeof(strLen))
		{
			Log::LogOnFile(Log::DEBUG_LEVEL, "CRecvBuffer remainSize Error\n");
			throw(_remainSize);
		}

		dequeueSize = _pBuf->Dequeue((char*)&strLen, sizeof(strLen));
		if (dequeueSize != sizeof(strLen))
		{
			Log::LogOnFile(Log::DEBUG_LEVEL, "CRecvBuffer dequeue Error\n");
			throw(dequeueSize);
		}
		_remainSize -= dequeueSize;

		if (_remainSize < strLen*sizeof(T::value_type))
		{
			Log::LogOnFile(Log::DEBUG_LEVEL, "CRecvBuffer remainSize Error\n");
			throw(_remainSize);
		}

		if (strLen > str.size())
		{
			str.resize(strLen);
		}


		dequeueSize = _pBuf->Dequeue((char*)str.data(), strLen * sizeof(T::value_type));
		if (dequeueSize != strLen * sizeof(T::value_type))
		{
			Log::LogOnFile(Log::DEBUG_LEVEL, "CRecvBuffer dequeue Error\n");
			throw(dequeueSize);
		}
		_remainSize -= dequeueSize;

		return *this;
	}

};