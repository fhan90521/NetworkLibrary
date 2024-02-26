#pragma once
#include "CRingBuffer.h"
#include "MyWindow.h"
#define DEFAULT_QUEUE_SIZE 1024

//T must have a copy constructor
//T must have a trivial assignment operator
//T must have a trivial destructor

template <typename T>
class SPSCQueue
{
private:
	CRingBuffer _queue;
public:
	SPSCQueue(int size= DEFAULT_QUEUE_SIZE) : _queue(size * sizeof(T)){};
	bool Enqueue(const T& inPar)
	{
		int enqueueSize = _queue.Enqueue((char*)&inPar, sizeof(T));
		if (enqueueSize != sizeof(T))
		{
			return false;
		}
		return true;
	}
	bool Dequeue(T* outPar)
	{
		int dequeueSize = _queue.Dequeue((char*)outPar, sizeof(T));
		if (dequeueSize != sizeof(T))
		{
			return false;
		}
		return true;
	}
	int Size()
	{
		int useSize = _queue.GetUseSize();
		return useSize / sizeof(T);
	}
};
