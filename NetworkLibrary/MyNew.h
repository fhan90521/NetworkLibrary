#pragma once
#include "Malloc.h"
#include "MyWindow.h"
#include <utility>
//#define CHECK_ALLOCATINGCNT

#ifdef CHECK_ALLOCATINGCNT
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
	
	static int GetAllocatingCnt()
	{
		return allocatingCnt;
	}
};
#endif

template<typename Type,typename ...Args>
Type* New(Args &&... args)
{
	Type* retP = (Type*)Malloc(sizeof(Type));
	new (retP) Type(std::forward<Args>(args)...);
#ifdef CHECK_ALLOCATINGCNT
	InterlockedIncrement(&GlobalObjectPool<Type>::allocatingCnt);
#endif
	return retP;
}

template<typename Type>
void Delete(Type* ptr)
{
#ifdef CHECK_ALLOCATINGCNT
	InterlockedDecrement(&GlobalObjectPool<Type>::allocatingCnt);
#endif
	ptr->~Type();
	Free(ptr);
}


