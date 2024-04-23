#pragma once
#include <utility>
#include <memory>
#include "MyNew.h"
#include "MyStlContainer.h"

template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args)
{
	SharedPtr<T> ptr = { New<T>(std::forward<Args>(args)...), Delete<T> };
	return ptr;
}