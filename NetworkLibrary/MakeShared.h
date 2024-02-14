#pragma once
#include <utility>
#include <memory>
#include "MyNew.h"

template<typename T>
using  SharedPtr = SharedPtr<T>;

template <typename T, typename... Args>
shared_ptr<T> MakeShared(Args&&... args)
{
	shared_ptr<T> ptr = { New<T>(forward<Args>(args)...), Delete<T> };
	return ptr;
}