#pragma once
#include "Malloc.h"
#include <utility>
using namespace std;
template <typename T>
class GlobalObjectPool
{
private:
	inline static LONG allocatingCnt = 0;
public:

	template<typename Type, typename ...Args>
	friend Type* New(Args &&... args);
	
	template<typename Type>
	friend void Delete(Type* ptr);
	
	template<typename Type>
	friend int GetAllocatingCnt();
};
template<typename Type, typename ...Args>
Type* New(Args &&... args)
{
	Type* retP = (Type*)Malloc(sizeof(Type));
	new (retP) Type(forward<Args>(args)...);
	GlobalObjectPool<Type>::allocatingCnt += 1;
	return retP;
}

template<typename Type>
void Delete(Type* ptr)
{
	GlobalObjectPool<Type>::allocatingCnt -= 1;
	ptr->~Type();
	Free(ptr);
}

template<typename Type>
inline int GetAllocatingCnt()
{
	return GlobalObjectPool<Type>::allocatingCnt;
}
