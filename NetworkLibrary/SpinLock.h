#pragma once
#include <Windows.h>
class SpinLock
{
private:
	CHAR _lock=false;
public:
	void Acquire()
	{
		while (1)
		{
			if (_lock == false)
			{
				if (InterlockedExchange8(&_lock, true) == false)
				{
					break;
				}
			}
		}
	}
	void Release()
	{
		_lock = false;
	}
};