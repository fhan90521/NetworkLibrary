#pragma once
#include "MyWindow.h"
#include <mutex>
#include <shared_mutex>

#define USE_LOCKS(count) std::shared_mutex _sMutex[count];
#define USE_LOCK	USE_LOCKS(1)
#define	SHARED_LOCK_IDX(idx) std::shared_lock sharedLockGuard##idx(_sMutex[idx]);
#define SHARED_LOCK	SHARED_LOCK_IDX(0)
#define	EXCLUSIVE_LOCK_IDX(idx) std::lock_guard exclusiveLockGuard##idx(_sMutex[idx]);
#define EXCLUSIVE_LOCK	EXCLUSIVE_LOCK_IDX(0)

#define USE_RECURSIVE_MUTEXS(count) std::recursive_mutex _rMutex[count];
#define USE_RECURSIVE_MUTEX USE_RECURSIVE_MUTEXS(1);
#define	RECURSIVE_LOCK_IDX(idx)	std::lock_guard recursiveLockGuard##idx(_rMutex[idx]);
#define RECURSIVE_LOCK	RECURSIVE_LOCK_IDX(0)
