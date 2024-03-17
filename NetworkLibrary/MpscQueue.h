#pragma once
#include "MyStlContainer.h"
#include "LockGuard.h"
#include <stdatomic.h>
#include "MyWindow.h"
template <typename T>
class MPSCQueue
{
private:
	Queue<T> _queue[2];
	std::atomic<int> _size = 0;
	char _enqueueIndex = 0;
	char _dequeueIndex = 1;
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
		_queue[_enqueueIndex].push(inPar);
		_size++;
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
		return true;
	}
	int Size()
	{
		return _size;
	}
};