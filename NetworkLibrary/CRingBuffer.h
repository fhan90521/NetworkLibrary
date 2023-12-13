#pragma once
#include<type_traits>
#define DEFAULT_RINGBUFFER_SIZE 0x2008
#define MAX_RINGBUFFER_SIZE	0x3000
class CRingBuffer
{
private:
	char* _buf;
	int _totalSize;
	int _front = 0;
	int _back = 0;
public:
	CRingBuffer(int size = DEFAULT_RINGBUFFER_SIZE) :_totalSize(size)
	{
		_buf = new char[size + 1];
	}
	~CRingBuffer()
	{
		delete[] _buf;
	}
	CRingBuffer(const CRingBuffer& src) = delete;
	CRingBuffer& operator=(const CRingBuffer& rhs) = delete;

	bool ReSize(int iSize);

	int GetBufferSize()
	{
		return _totalSize;
	}
	int GetUseSize()
	{
		int front = _front;
		int back = _back;
		if (front <= back)
		{
			return back - front;
		}
		else
		{
			return back + (_totalSize - front + 1);
		}
	}
	int GetFreeSize()
	{
		return _totalSize - GetUseSize();
	}

	int DirectEnqueueSize()
	{
		int front = _front;
		if (front <= _back)
		{
			if (front == 0)
			{
				return _totalSize - _back;
			}
			else
			{
				return _totalSize - _back + 1;
			}
		}
		else
		{
			return front - _back - 1;
		}
	}
	int DirectDequeueSize()
	{
		int back = _back;
		if (_front <= back)
		{
			return back - _front;
		}
		else
		{
			return _totalSize - _front + 1;
		}
	}
	char* GetFrontBufferPtr()
	{
		return &_buf[_front];
	}
	char* GetBackBufferPtr()
	{
		return &_buf[_back];
	}
	char* GetBufferPtr()
	{
		return _buf;
	}
	void ClearBuffer()
	{
		_back = _front;
	}

	int Enqueue(char* data, int iSize);
	int Dequeue(char* dest, int iSize);

	int Peek(char* dest, int iSize);

	int MoveBack(int iSize);
	int MoveFront(int iSize);

	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	CRingBuffer& operator >> (T& data) throw(int)
	{
		Dequeue((char*) & data, sizeof(T));
		return *this;
	}
};