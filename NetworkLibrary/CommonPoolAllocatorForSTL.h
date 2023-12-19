#pragma once

//allocate 안에서 return (T*)(CommonMemoryPool::GetInstance()->Alloc(sizeof(T) * n)); 호출 할 때
//long long-> int 형변환 발생 int 최대값보다 큰 메모리 할당을 할일이 없어서 무시
#pragma warning(disable : 4267)



#include "CommonMemoryPool.h"
//#include <typeinfo>
using namespace std;
template <class T>
class CommonPoolAllocatorForSTL
{
public:

	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T value_type;

	CommonPoolAllocatorForStd() = default;

	template <class U>
	struct rebind
	{
		typedef CommonPoolAllocatorForStd<U> other;
	};
	template<typename U>
	CommonPoolAllocatorForStd(const CommonPoolAllocatorForStd<U>& other) {};


	pointer allocate(size_type n)
	{
		//std::cout << typeid(T).name() << std::endl;
		//std::cout << sizeof(T)* n<<std::endl;
		T* ret = (T*)(CommonMemoryPool::GetInstance()->Alloc(sizeof(T) * n));
		return ret;
	}

	void deallocate(pointer p, size_type n)
	{
		CommonMemoryPool::GetInstance()->Free(p);
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


