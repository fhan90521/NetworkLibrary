#pragma once
#include "LockGuard.h"
#include "MyStlContainer.h"
template<typename T>
class LockQueue
{
public:
	void Enqueue(const T& inPar)
	{
		EXCLUSIVE_LOCK;
		_queue.push(inPar);
	}
	bool Dequeue(T* outPar)
	{
		EXCLUSIVE_LOCK;
		if (_queue.empty())
		{
			return false;
		}
		*outPar = _queue.front();
		_queue.pop();
		return true;
	}
	int Size()
	{
		return _queue.size();
	}

private:
	USE_LOCK
	Queue<T> _queue;
};