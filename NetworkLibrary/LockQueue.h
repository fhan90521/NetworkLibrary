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

	bool Pop(T* outPar)
	{
		EXCLUSIVE_LOCK;
		if (_queue.empty())
			return false;

		*outPar = _queue.front();
		_queue.pop();
		return true;
	}

	int PopAll(Vector<T>& outVec)
	{
		int popCnt = 0;
		EXCLUSIVE_LOCK;
		while (_queue.empty()==false)
		{
			outVec.push_back(_queue.front());
			_queue.pop();
			popCnt++;
		}
		return popCnt;
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