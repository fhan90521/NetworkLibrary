#pragma once
#include "Memory/MyNew.h"
#include "Container/MyStlContainer.h"
#include <utility>

template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args)
{
	SharedPtr<T> ptr = { New<T>(std::forward<Args>(args)...), Delete<T> };
	return ptr;
}