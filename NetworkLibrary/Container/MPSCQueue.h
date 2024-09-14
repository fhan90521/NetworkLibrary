#pragma once
#include "OS/MyWindow.h"
#include "Container/MyStlContainer.h"
#include "Lock/LockGuard.h"

#include <atomic>
template <typename T>
class MPSCQueue
{
private:
	struct alignas(64) AlignQueue
	{
		Queue<T> queue;
		std::atomic<size_t> size=0;
	};
	AlignQueue _queues[2];
	alignas(64) SRWLOCK _srwLock;
	char _enqueueIndex = 0;
	alignas(64) char _dequeueIndex = 1;
	void Swap()
	{
		SRWLockGuard<LOCK_TYPE::EXCLUSIVE> srwLockGuard(_srwLock);
		std::swap(_enqueueIndex, _dequeueIndex);
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
		_queues[_enqueueIndex].size++;
	}
	bool Dequeue(T* outPar)
	{
		if (_queues[_dequeueIndex].size == 0)
		{
			if (_queues[_enqueueIndex].size == 0)
			{
				return false;
			}
			Swap();
		}
		*outPar = _queues[_dequeueIndex].queue.front();
		_queues[_dequeueIndex].queue.pop();
		_queues[_dequeueIndex].size--;
		return true;
	}
	size_t Size()
	{
		return _queues[0].size + _queues[1].size;
	}
};