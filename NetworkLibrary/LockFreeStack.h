#pragma once
#include "TlsObjectPool.h"
#include "MyWindow.h"

template<typename T>
class LockFreeStack
{
private:
	//최적화 컴파일을 켜지 않았을 때 정상작동
	union Top
	{
		struct BitPartial
		{
			LONG64 pNode : 47;
			LONG64 cnt : 17;
		};
		BitPartial bitPartial;
		LONG64 entire;
	};
	struct Node
	{
		T data;
		Node* pNext = nullptr;
	};
	alignas(64) Top _top;
	alignas(64) LONG _size;
	static TlsObjectPool<Node,true> _objectPool;
public:
	LockFreeStack()
	{
		_top.entire = NULL;
		_size = 0;
	};
	~LockFreeStack()
	{
		T tmp;
		while (Pop(&tmp));
	}
	void Push(const T& inPar)
	{
		Node* pNewNode = _objectPool.Alloc();
		pNewNode->data = inPar;
		Top curTop;
		Top newTop;
		newTop.bitPartial.pNode = (LONG64)pNewNode;
		while (1)
		{
			curTop = _top;
			pNewNode->pNext = (Node*)curTop.bitPartial.pNode;
			newTop.bitPartial.cnt = curTop.bitPartial.cnt + 1;
			if (InterlockedCompareExchange64(&_top.entire, newTop.entire, curTop.entire) == curTop.entire)
			{
				break;
			}
		}
		InterlockedIncrement(&_size);
	}
	bool Pop(T* outPar)
	{
		if (InterlockedDecrement(&_size) < 0)
		{
			InterlockedIncrement(&_size);
			return false;
		}
		Top curTop;
		Top newTop;
		while (1)
		{
			curTop = _top;
			if (curTop.bitPartial.pNode == NULL)
			{
				return false;
			}
			newTop.entire = (LONG64)((Node*)(curTop.bitPartial.pNode))->pNext;
			newTop.bitPartial.cnt = curTop.bitPartial.cnt;
			if (InterlockedCompareExchange64(&_top.entire, newTop.entire, curTop.entire) == curTop.entire)
			{
				*outPar = ((Node*)curTop.bitPartial.pNode)->data;
				_objectPool.Free(((Node*)curTop.bitPartial.pNode));
				break;
			}
		}
		return true;
	}
	LONG Size()
	{
		return _size;
	}
};
template<typename T>
TlsObjectPool<typename LockFreeStack<T>::Node, true> LockFreeStack<T>::_objectPool;