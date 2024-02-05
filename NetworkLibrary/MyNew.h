#pragma once
#include "Malloc.h"
#include <utility>

using namespace std;
template<typename T, typename ...Args>
T* New(Args &&... args)
{
	T* retP =(T*)Malloc(sizeof(T));
	new (retP) T(forward<Args>(args)...);
	return retP;
}

template<typename T>
void Delete(T* ptr)
{
	ptr->~T();
	Free(ptr);
}
