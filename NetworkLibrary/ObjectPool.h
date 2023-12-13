#include <new>
#include <list>
#include <iostream>
#include <Windows.h>
#include "GetPOOLID.h"
#include <utility>

#define ATTATCH_TAIL
#define COOKIE_VALUE (short)0xAAAA
template <class Type, typename... Args>
class ObjectPool
{
private:
	struct NodeTail
	{
		short cookie;
		unsigned short pool_id;
	};
	struct Node
	{
		Type data;
#ifdef ATTATCH_TAIL
		NodeTail nodeTail;
#endif
		Node* next = nullptr;
	};

	Node* _pFreeNode = nullptr;

	unsigned short _pool_id;
	int _nodePerBlock;
	int _allocatingCnt = 0;
	bool _allocPlacementNew;

	std::list<Node*> pBlockBeginList;

	ObjectPool(const ObjectPool& src) = delete;
	ObjectPool& operator=(const ObjectPool& rhs) = delete;

	void AllocBlock()
	{
		Node* pBlockBegin = (Node*)malloc(_nodePerBlock * sizeof(Node));
		pBlockBeginList.push_back(pBlockBegin);

		Node* pNode = pBlockBegin;
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
#ifdef ATTATCH_TAIL
			(pNode->nodeTail).pool_id = _pool_id;
			(pNode->nodeTail).cookie = COOKIE_VALUE;
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
		_pool_id = GetPOOLID();
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
#ifdef ATTATCH_TAIL
		if ((pNode->nodeTail).cookie != COOKIE_VALUE)
		{
			cout << "object pool cookie modulation" << endl;
			DebugBreak();
		}
		else if ((pNode->nodeTail).pool_id != _pool_id)
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













