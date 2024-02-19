#pragma once
#include <shared_mutex>
//class SharedMutex
//{
//private:
//	alignas(64) std::shared_mutex _mutex;
//public:
//	void LockShared() { _mutex.lock_shared();}
//	void UnlockShared() { _mutex.unlock_shared(); }
//	void Lock() { _mutex.lock(); }
//	void UnLock() { _mutex.unlock(); }
//};

class SharedLockGuard
{
public:
	SharedLockGuard(std::shared_mutex& sMutex) : _sMutex(sMutex){ _sMutex.lock_shared(); }
	~SharedLockGuard() { _sMutex.unlock_shared(); }

private:
	std::shared_mutex& _sMutex;
};

class ExclusiveLockGuard
{
public:
	ExclusiveLockGuard(std::shared_mutex& sMutex) : _sMutex(sMutex) {_sMutex.lock();}
	~ExclusiveLockGuard() {_sMutex.unlock();}
private:
	std::shared_mutex& _sMutex;
};

#define USE_MUTEXS(count)	std::shared_mutex _sMutex[count];
#define USE_MUTEX	USE_MUTEXS(1)
#define	SHARED_LOCK_IDX(idx)	SharedLockGuard sharedLockGuard##idx(_sMutex[idx]);
#define SHARED_LOCK	SHARED_LOCK_IDX(0)
#define	EXCLUSIVE_LOCK_IDX(idx)	ExclusiveLockGuard exclusiveLockGuard##idx(_sMutex[idx]);
#define EXCLUSIVE_LOCK	EXCLUSIVE_LOCK_IDX(0)