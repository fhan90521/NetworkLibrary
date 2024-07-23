#pragma once
#pragma warning(disable : 4267)
#include "Malloc.h"
#include <cstddef>

template<typename T>
class PoolAllocatorForSTL
{
public:
	using value_type = T;

	PoolAllocatorForSTL() { }

	template<typename Other>
	PoolAllocatorForSTL(const PoolAllocatorForSTL<Other>&) { }

	T* allocate(size_t count)
	{
		//std::cout << typeid(T).name() << std::endl;
		//std::cout << sizeof(T)* n<<std::endl;
		T* ret = (T*)(Malloc(sizeof(T) * count));
		return ret;
	}

	void deallocate(T* p, size_t n)
	{
		Free(p);
	}
};


