#pragma once
#pragma warning(disable : 4267)
#include "Malloc.h"
//#include <typeinfo>
using namespace std;
template <class T>
class PoolAllocatorForSTL
{
public:

	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T value_type;

	PoolAllocatorForSTL() = default;

	template <class U>
	struct rebind
	{
		typedef PoolAllocatorForSTL<U> other;
	};
	template<typename U>
	PoolAllocatorForSTL(const PoolAllocatorForSTL<U>& other) {};


	pointer allocate(size_type n)
	{
		//std::cout << typeid(T).name() << std::endl;
		//std::cout << sizeof(T)* n<<std::endl;
		T* ret = (T*)(Malloc(sizeof(T) * n));
		return ret;
	}

	void deallocate(pointer p, size_type n)
	{
		Free(p);
	}

	void construct(pointer p, const_reference val)
	{
		new (p) T(val);
	}

	void destroy(pointer p)
	{
		p->~T();
	}
};


