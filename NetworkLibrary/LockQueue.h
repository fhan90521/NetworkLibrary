#pragma once
#include "LockGuard.h"
#include "MyStlContainer.h"
template<typename T>
class LockQueue
{
public:
	void Enqueue(const T& inPar)
	{
		SRWLockGuard<LOCK_TYPE::EXCLUSIVE> srwLockGuard(_srwLock);
		_queue.push(inPar);
		_size++;
	}
	bool Dequeue(T* outPar)
	{
		SRWLockGuard<LOCK_TYPE::EXCLUSIVE> srwLockGuard(_srwLock);
		if (_queue.empty())
		{
			return false;
		}
		*outPar = _queue.front();
		_queue.pop();
		_size--;
		return true;
	}
	size_t Size()
	{
		return _size;
	}
	LockQueue()
	{
		InitializeSRWLock(&_srwLock);
	}
private:
	SRWLOCK _srwLock;
	Queue<T> _queue;
	size_t _size;
};