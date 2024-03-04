#pragma once
#include "LockFreeObjectPool.h"
#include "GetMyThreadID.h"
#include "MyWindow.h"
#include <utility>
#include <new.h>
#include <iostream>
template <typename T, bool _bPlacementNew>
class TlsObjectPool
{
private:
	enum
	{
		THREADCNT = 16
	};

	struct Node
	{
		T data;
		Node* pNext;
	};

	struct NodeBlock :SLIST_ENTRY
	{
	private:
		void* allocPtr = nullptr;
		int _nodePerBlock;
	public:
		Node* pTopNode = nullptr;
		NodeBlock(int nodePerBlock): _nodePerBlock(nodePerBlock)
		{
			void* pBlock = malloc(nodePerBlock * sizeof(Node));
			Node* pNode = nullptr;
			for (int i = 0; i < nodePerBlock; i++)
			{
				pNode = (Node*)((char*)pBlock + (i * sizeof(Node)));
				if constexpr (_bPlacementNew == false)
				{
					new ((T*)pNode)T;
				}
				if (i == nodePerBlock - 1)
				{
					pNode->pNext = nullptr;
				}
				else
				{
					pNode->pNext = (Node*)((char*)pNode + sizeof(Node));
				}
			}
			allocPtr = pBlock;
			pTopNode = (Node*)pBlock;
		}
		~NodeBlock()
		{
			if constexpr (_bPlacementNew == false)
			{
				Node* nodeBlock = (Node*)allocPtr;
				for (int i = 0; i < _nodePerBlock; i++)
				{
					nodeBlock[i].data.~T();
				}
			}
			free(allocPtr);
		}
	};

	struct alignas(64) PoolState
	{
		Node* pTopNode = nullptr;
		Node* pFreePrev = nullptr;
		int remainCnt = 0;
		int allocatingCnt = 0;
	};


	PoolState _poolStateArr[THREADCNT];
	LockFreeObjectPool<NodeBlock, false> _blockPool;
	SLIST_HEADER _blockListHead;
	int _nodePerBlock;

	void AllocBlock()
	{
		PoolState& poolState = _poolStateArr[GetMyThreadID()];
		NodeBlock* pBlock = _blockPool.Alloc(_nodePerBlock);
		poolState.pTopNode = pBlock->pTopNode;
		poolState.remainCnt = _nodePerBlock;
		InterlockedPushEntrySList(&_blockListHead, pBlock);
	}

	void FreeBlock()
	{
		PoolState& poolState = _poolStateArr[GetMyThreadID()];
		NodeBlock* pOldTopBlock = (NodeBlock*)InterlockedPopEntrySList(&_blockListHead);
		pOldTopBlock->pTopNode = poolState.pFreePrev->pNext;
		poolState.pFreePrev->pNext = nullptr;
		poolState.remainCnt = _nodePerBlock;
		_blockPool.Free(pOldTopBlock);
	}
public:
	TlsObjectPool(int nodePerBlock = 512)
	{
		InitializeSListHead(&_blockListHead);
		_nodePerBlock = nodePerBlock;
	}
	~TlsObjectPool()
	{
		NodeBlock* pBlock;
		while (pBlock = (NodeBlock*)InterlockedPopEntrySList(&_blockListHead))
		{
			pBlock->pTopNode = nullptr;
			_blockPool.Free(pBlock);
		}
	}
	template<typename... Args>
	T* Alloc(Args&&... args)
	{
		PoolState& poolState = _poolStateArr[GetMyThreadID()];
		if (poolState.pTopNode == nullptr)
		{
			AllocBlock();
		}

		Node* pOldTop = poolState.pTopNode;
		poolState.pTopNode = pOldTop->pNext;
		poolState.remainCnt--;
		poolState.allocatingCnt++;
		if constexpr (_bPlacementNew)
		{
			new ((T*)pOldTop)T(std::forward<Args>(args)...);
		}
		return (T*)pOldTop;
	}
	T* Alloc(void)
	{
		PoolState& poolState = _poolStateArr[GetMyThreadID()];
		if (poolState.pTopNode == nullptr)
		{
			AllocBlock();
		}
		Node* pOldTop = poolState.pTopNode;
		poolState.pTopNode = pOldTop->pNext;
		poolState.remainCnt--;
		poolState.allocatingCnt++;
		if constexpr (_bPlacementNew)
		{
			new ((T*)pOldTop)T;
		}
		return (T*)pOldTop;
	}

	void Free(T* pData)
	{
		if constexpr (_bPlacementNew)
		{
			pData->~T();
		}
		Node* pNewTop = (Node*)pData;
		PoolState& poolState = _poolStateArr[GetMyThreadID()];

		pNewTop->pNext = poolState.pTopNode;
		poolState.pTopNode = pNewTop;
		poolState.remainCnt++;
		poolState.allocatingCnt--;

		if (poolState.remainCnt == _nodePerBlock + 1)
		{
			poolState.pFreePrev = poolState.pTopNode;
		}

		if (poolState.remainCnt == _nodePerBlock * 2)
		{
			FreeBlock();
		}
	}
	LONG GetAllocCnt()
	{
		LONG ret = 0;
		for (int i = 0; i < THREADCNT; i++)
		{
			ret += _poolStateArr[i].allocatingCnt;
		}
		return ret;
	}
};