#pragma once
#include <utility>
#include <memory>
#include "MyNew.h"

template <typename T, typename... Args>
auto MakeUnique(Args&&... args)
{
	auto CustomDeleter = [](T* ptr)
	{
		Delete<T>(ptr);
	};
	std::unique_ptr<T, decltype(CustomDeleter)> ptr(New<T>(forward<Args>(args)...), CustomDeleter);
	return ptr;
}