#include <new>
#include <iostream>
#include <Windows.h>
#include "GetPOOLID.h"
#include <utility>
using namespace std;
#define ADD_CHECK
#define COOKIE_VALUE (short)0xAAAA
template <class Type>
class ObjectPool
{
private:
	struct Node
	{
		Type data;
#ifdef ADD_CHECK
		short cookie;
		unsigned short id;
#endif
		Node* next = nullptr;
	};

	Node* _pFreeNode = nullptr;
	unsigned short _id;
	int _allocatingCnt = 0;
	bool _allocPlacementNew;
	ObjectPool(const ObjectPool& src) = delete;
	ObjectPool& operator=(const ObjectPool& rhs) = delete;
public:

	ObjectPool(bool allocPlacementNew): _allocPlacementNew(allocPlacementNew)
	{
		_id = GetPOOLID();
	}
	virtual	~ObjectPool()
	{
		
		while (Node* pCurTop = _pFreeNode)
		{
			_pFreeNode = _pFreeNode->next;
			if (_allocPlacementNew==false)
			{
				((Type*)pCurTop)->~Type();
			}
			free(pCurTop);
		}
		
	}

	template <typename... Args>
	Type* Alloc(Args&& ...args)
	{
		Type* retP = (Type*)_pFreeNode;
		if (retP != nullptr)
		{
			_pFreeNode = _pFreeNode->next;
			if (_allocPlacementNew)
			{
				new (retP) Type(std::forward<Args>(args)...);
			}
		}
		else
		{
			retP =(Type*)malloc(sizeof(Node));
			new (retP) Type(std::forward<Args>(args)...);
#ifdef ADD_CHECK
			((Node*)retP)->cookie = COOKIE_VALUE;
			((Node*)retP)->id = _id;
#endif
		}
		
		_allocatingCnt++;
		return retP;
	}

	void Free(Type* pData)
	{
		Node* pNode = (Node*)pData;
#ifdef ADD_CHECK
		if (pNode->cookie != COOKIE_VALUE)
		{
			cout << "object pool cookie modulation" << endl;
			DebugBreak();
		}
		else if (pNode->id !=_id)
		{
			cout << "pool id not match" << endl;
			DebugBreak();
		}
#endif
		pNode->next = _pFreeNode;
		_pFreeNode = pNode;

		if (_allocPlacementNew)
		{
			pData->~Type();
		}
		_allocatingCnt--;
	}
	int		GetAllocatingCnt() { return _allocatingCnt; }
};













