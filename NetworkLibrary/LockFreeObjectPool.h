#pragma once
#include "MyWindow.h"
#include <utility>
template <typename T ,bool _bPlacementNew>
class LockFreeObjectPool
{
private:
	union Top
	{
		struct BitPartial
		{
			LONG64 pTopNode : 47;
			LONG64 cnt : 17;
		}bitPartial;
		LONG64 entire = NULL;
	};
	struct Node
	{
		T data;
		Node* pNext = nullptr;
	};
	alignas(64)Top _top;
public:
	LockFreeObjectPool() {};
	virtual	~LockFreeObjectPool() 
	{
		Node* pNode = (Node*)(_top.bitPartial.pTopNode);
		while (pNode)
		{
			T* pData = (T*)pNode;
			if constexpr (_bPlacementNew==false)
			{
				pData->~T();
			}

			Node* pDelete = pNode;
			pNode = pNode->pNext;
			_aligned_free(pDelete);
		}
	};
	//가변인자 템플릿이 인자를 void로 할때 rep stos byte ptr [rdi] 어셈블리어로 할당해주는 주소를 0으로 밀어주는데
	//이게 락프리큐(LockFreeQueueBasic)에서 두개이상의 스레드가 할당해주는 주소에 쓰기를 시도할 때  byte단위로 mov를 해서 문제를 일으킬 수 있다. 
	template<typename... Args>
	T* Alloc(Args&&... args)
	{
		Node* pNode = nullptr;
		Top oldTop;
		Top newTop;
		while (1)
		{
			oldTop.entire = _top.entire;
			if (oldTop.bitPartial.pTopNode == NULL)
			{
				pNode = (Node*)_aligned_malloc(sizeof(Node),16);
				//이부분에서 void 일때 rep stos byte ptr [rdi]
				new (pNode)T(std::forward<Args>(args)...);
				return (T*)pNode;
			}
			newTop.entire = (LONG64)(((Node*)(oldTop.bitPartial.pTopNode))->pNext);
			newTop.bitPartial.cnt = oldTop.bitPartial.cnt + 1;
			if (InterlockedCompareExchange64(&_top.entire, newTop.entire, oldTop.entire) == oldTop.entire)
			{
				break;
			}
		}
		pNode = (Node*)oldTop.bitPartial.pTopNode;
		if constexpr (_bPlacementNew)
		{
			new (pNode)T(std::forward<Args>(args)...);
		}
		return (T*)pNode;
	}
	T* Alloc(void)
	{
		Node* pNode = nullptr;
		Top oldTop;
		Top newTop;
		while (1)
		{
			oldTop.entire = _top.entire;
			if (oldTop.bitPartial.pTopNode == NULL)
			{
				pNode = (Node*)_aligned_malloc(sizeof(Node),16);
				new (pNode)T;
				return (T*)pNode;
			}
			newTop.entire = (LONG64)(((Node*)(oldTop.bitPartial.pTopNode))->pNext);
			newTop.bitPartial.cnt = oldTop.bitPartial.cnt + 1;
			if (InterlockedCompareExchange64(&_top.entire, newTop.entire, oldTop.entire) == oldTop.entire)
			{
				break;
			}
		}
		pNode = (Node*)oldTop.bitPartial.pTopNode;
		if constexpr (_bPlacementNew)
		{
			new (pNode)T;
		}
		return (T*)pNode;
	}

	bool Free(T* pData)
	{
		if constexpr (_bPlacementNew)
		{
			pData->~T();
		}
		Node* pNode = (Node*)pData;
		Top oldTop;
		Top newTop;
		newTop.entire = (LONG64)pData;
		while (1)
		{
			oldTop.entire = _top.entire;
			pNode->pNext = (Node*)(oldTop.bitPartial.pTopNode);
			newTop.bitPartial.cnt = oldTop.bitPartial.cnt + 1;
			if (InterlockedCompareExchange64(&_top.entire, newTop.entire, oldTop.entire) == oldTop.entire)
			{
				break;
			}
		}
		return true;
	}
};






