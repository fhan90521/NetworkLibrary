#pragma once
#include "CRingBuffer.h"
#include "MyWindow.h"
#define DEFAULT_QUEUE_SIZE 1024
template <typename T>
class CLockQueue
{
private:
	CRingBuffer _queue;
	SRWLOCK _lock;
public:
	CLockQueue(int size= DEFAULT_QUEUE_SIZE) : _queue(size * sizeof(T))
	{
		InitializeSRWLock(&_lock);
	};
	bool Push(const T& inPar)
	{
		AcquireSRWLockExclusive(&_lock);
		int enqueueSize = _queue.Enqueue((char*)&inPar, sizeof(T));
		ReleaseSRWLockExclusive(&_lock);
		if (enqueueSize != sizeof(T))
		{
			return false;
		}
		return true;
	}
	bool Pop(T& outPar)
	{
		AcquireSRWLockExclusive(&_lock);
		int dequeueSize = _queue.Dequeue((char*)&outPar, sizeof(T));
		ReleaseSRWLockExclusive(&_lock);
		if (dequeueSize != sizeof(T))
		{
			return false;
		}
		return true;
	}
	int Peek(T* buf, int peekCnt)
	{
		AcquireSRWLockShared(&_lock);
		int peekSize = _queue.Peek((char*)buf, peekCnt * sizeof(T));
		ReleaseSRWLockShared(&_lock);
		return peekSize / sizeof(T);
	}
	bool NoLockPush(const T& inPar)
	{
		int enqueueSize = _queue.Enqueue((char*)&inPar, sizeof(T));
		if (enqueueSize != sizeof(T))
		{
			return false;
		}
		return true;
	}
	bool NoLockPop(T& outPar)
	{
		int dequeueSize = _queue.Dequeue((char*)&outPar, sizeof(T));
		if (dequeueSize != sizeof(T))
		{
			return false;
		}
		return true;
	}
	int NoLockPeek(T* buf, int peekCnt)
	{
		int peekSize = _queue.Peek((char*)buf, peekCnt * sizeof(T));
		return peekSize / sizeof(T);
	}
	int GetSize()
	{
		int useSize = _queue.GetUseSize();
		return useSize / sizeof(T);
	}
};
