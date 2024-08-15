#pragma once
#include "LockGuard.h"
#include "MyStlContainer.h"
template<typename T>
class LockStack
{
public:
	void Push(const T& inPar)
	{
		SRWLockGuard<LOCK_TYPE::EXCLUSIVE> srwLockGuard(_srwLock);
		_stack.push(inPar);
		_size++;
	}
	bool Pop(T* outPar)
	{
		SRWLockGuard<LOCK_TYPE::EXCLUSIVE> srwLockGuard(_srwLock);
		if (_stack.empty())
		{
			return false;
		}
		*outPar = _stack.top();
		_stack.pop();
		_size--;
		return true;
	}
	size_t Size()
	{
		return _size;
	}
	LockStack()
	{
		InitializeSRWLock(&_srwLock);
	}
private:
	SRWLOCK _srwLock;
	Stack<T> _stack;
	size_t _size;
};