#pragma once
#include "Memory/MyNew.h"
#include "Container/MyStlContainer.h"
#include <utility>

template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args)
{
#ifdef CHECK_ALLOCATINGCNT
	SharedPtr<T> ptr = { New<T>(std::forward<Args>(args)...), Delete<T> };
#else
	SharedPtr<T> ptr = std::allocate_shared<T>(PoolAllocatorForSTL<T>(), std::forward<Args>(args)...);
#endif
	return ptr;
}