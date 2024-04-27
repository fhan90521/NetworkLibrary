#pragma once
#include "LockGuard.h"
#include "MyStlContainer.h"
template<typename T>
class LockQueue
{
public:
	void Push(const T& inPar)
	{
		EXCLUSIVE_LOCK;
		_queue.push(inPar);
	}

	T Pop()
	{
		EXCLUSIVE_LOCK;
		if (_queue.empty())
			return T();
		T ret = _queue.front();
		_queue.pop();
		return ret;
	}

	void PopAll(Vector<T>& outVec)
	{
		EXCLUSIVE_LOCK;
		for(int i = 0 ;i<_queue.size();i++)
		{
			outVec.push_back(Pop());
		}
	}

	void Clear()
	{
		EXCLUSIVE_LOCK;
		_queue = Queue<T>();
	}

private:
	USE_MUTEX
	Queue<T> _queue;
};