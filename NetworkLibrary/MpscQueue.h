#pragma once
#include "MyStlContainer.h"
#include "LockGuard.h"
#include <stdatomic.h>
#include "MyWindow.h"
template <typename T>
class MPSCQueue
{
private:
	struct AlignQueue
	{
		alignas(64) Queue<T> queue;
	};
	AlignQueue _queues[2];
	SRWLOCK _srwLock;
	char _enqueueIndex = 0;
	alignas(64) char _dequeueIndex = 1;
	bool Flip()
	{
		SRWLockGuard<LOCK_TYPE::EXCLUSIVE> srwLockGuard(_srwLock);
		if (_queues[_enqueueIndex].queue.size()== 0)
		{
			return false;
		}
		std::swap(_enqueueIndex, _dequeueIndex);
		return true;
	}
public:
	MPSCQueue()
	{
		InitializeSRWLock(&_srwLock);
	}
	void Enqueue(const T& inPar)
	{
		SRWLockGuard<LOCK_TYPE::EXCLUSIVE> srwLockGuard(_srwLock);
		_queues[_enqueueIndex].queue.push(inPar);
	}
	bool Dequeue(T* outPar)
	{
		if (_queues[_dequeueIndex].queue.size() == 0)
		{
			if (Flip() == false)
			{
				return false;
			}
		}
		*outPar = _queues[_dequeueIndex].queue.front();
		_queues[_dequeueIndex].queue.pop();
		return true;
	}
	int Size()
	{
		return _queues[0].queue.size()+_queues[1].queue.size();
	}
};