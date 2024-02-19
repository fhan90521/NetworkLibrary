#pragma once
#include "MyStlContainer.h"
#include "LockGuard.h"
#include <stdatomic.h>
template <typename T>
class MpscQueue
{
private:
	Queue<T> _queue[2];
	char _enqueueIndex = 0;
	char _dequeueIndex = 1;
	std::atomic<int> _size = 0;
	USE_MUTEX;
	void Flip()
	{
		EXCLUSIVE_LOCK
		std::swap(_enqueueIndex, _dequeueIndex);
	}
public:
	void Enqueue(const T& inPar)
	{
		EXCLUSIVE_LOCK;
		_size++;
		_queue[_enqueueIndex].push(inPar);
	}
	bool Dequeue(T* outPar)
	{
		if (_size == 0)
		{
			return false;
		}
		_size--;
		if (_queue[_dequeueIndex].size() == 0)
		{
			Flip();
		}
		*outPar = std::move(_queue[_dequeueIndex].front());
		_queue[_dequeueIndex].pop();
	}
	int Size()
	{
		return _size;
	}
};