#pragma once
#include "MyWindow.h"
#include <mutex>
enum class LOCK_TYPE
{
	SHARED,
	EXCLUSIVE,
};
template <LOCK_TYPE type>
class SRWLockGuard
{
public:
	SRWLockGuard(SRWLOCK& srwLock) : _srwLock(srwLock)
	{ 
		if constexpr (type == LOCK_TYPE::SHARED)
		{
			AcquireSRWLockShared(&_srwLock);
		}
		else
		{

			AcquireSRWLockExclusive(&_srwLock);
		}
	}
	~SRWLockGuard()
	{
		if constexpr (type == LOCK_TYPE::SHARED)
		{
			ReleaseSRWLockShared(&_srwLock);
		}
		else
		{
			ReleaseSRWLockExclusive(&_srwLock);
		}
	}

private:
	SRWLOCK& _srwLock;
};

#define USE_LOCKS(count)	SRWLOCK _srwLock[count];
#define USE_LOCK	USE_LOCKS(1)
#define	SHARED_LOCK_IDX(idx)	SRWLockGuard<LOCK_TYPE::SHARED> sharedLockGuard##idx(_srwLock[idx]);
#define SHARED_LOCK	SHARED_LOCK_IDX(0)
#define	EXCLUSIVE_LOCK_IDX(idx)	SRWLockGuard<LOCK_TYPE::EXCLUSIVE> exclusiveLockGuard##idx(_srwLock[idx]);
#define EXCLUSIVE_LOCK	EXCLUSIVE_LOCK_IDX(0)

#define USE_RECURSIVE_MUTEXS(count) std::recursive_mutex _rMutex[count];
#define USE_RECURSIVE_MUTEX USE_RECURSIVE_MUTEXS(1);
#define	RECURSIVE_LOCK_IDX(idx)	std::lock_guard recursiveLockGuard##idx(_rMutex[idx]);
#define RECURSIVE_LOCK	RECURSIVE_LOCK_IDX(0)
