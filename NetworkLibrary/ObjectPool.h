#include <new>
#include <list>
#include <iostream>
#include <Windows.h>
#include "GetPOOLID.h"
#include <utility>
using namespace std;
//#define ADD_CHECK
#define COOKIE_VALUE (short)0xAAAA
template <class Type, typename... Args>
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
	int _nodePerBlock;
	int _allocatingCnt = 0;
	bool _allocPlacementNew;

	std::list<void*> pBlockBeginList;

	ObjectPool(const ObjectPool& src) = delete;
	ObjectPool& operator=(const ObjectPool& rhs) = delete;

	void AllocBlock()
	{
		void* pBlockBegin = (void*)malloc(_nodePerBlock * sizeof(Node));
		pBlockBeginList.push_back(pBlockBegin);

		Node* pNode = (Node*)pBlockBegin;
		for (int i = 0; i < _nodePerBlock; i++)
		{
			if (_allocPlacementNew == false)
			{
				new (&(pNode->data)) Type;
			}

			if (i == _nodePerBlock - 1)
			{
				pNode->next = nullptr;
			}
			else
			{
				pNode->next = pNode + 1;
			}
#ifdef ADD_CHECK
			pNode->cookie = COOKIE_VALUE;
			pNode->id = _id;
#endif
			pNode++;
		}
		_pFreeNode = pBlockBegin;
	}


	void Clear()
	{

		for (Node* pBlockBegin : pBlockBeginList)
		{
			if (_allocPlacementNew == false)
			{
				Node* pNode = pBlockBegin;
				for (int i = 0; i < _nodePerBlock; i++)
				{
					(pNode->data).~Type();
					pNode++;
				}
			}
			free(pBlockBegin);
		}
	}
public:

	ObjectPool(int nodePerBlock, bool allocPlacementNew)
		: _nodePerBlock(nodePerBlock), _allocPlacementNew(allocPlacementNew)
	{
		_id = GetPOOLID();
	}
	virtual	~ObjectPool()
	{
		Clear();
	}

	Type* Alloc(Args&& ...args)
	{
		if (_pFreeNode == nullptr)
		{
			AllocBlock();
		}
		Type* retP = (Type*)_pFreeNode;
		_pFreeNode = _pFreeNode->next;
		if (_allocPlacementNew)
		{
			new (retP) Type(std::forward<Args>(args)...);
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

	int		GetCapacityCnt() { return pBlockBeginList.size() * _nodePerBlock; }
	int		GetAllocatingCnt() { return _allocatingCnt; }
};













