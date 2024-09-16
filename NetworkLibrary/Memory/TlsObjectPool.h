#pragma once
#include "OS/MyWindow.h"
#include "Container/MyStlContainer.h"
#include "Container/LockQueue.h"
#include <utility>
#include <new.h>
template <typename T, bool _bPlacementNew>
class TlsObjectPool
{
private:
	struct Node
	{
		T data;
		Node* pNext;
	};

	DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT)
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
				pNode->pNext = (Node*)((char*)pNode + sizeof(Node));
			}
			pNode->pNext = nullptr;

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

	struct alignas(64) PoolState :SLIST_ENTRY
	{
		Node* pTopNode = nullptr;
		Node* pFreePrev = nullptr;
		int remainCnt = 0;
		int allocatingCnt = 0;
	};

	
	
	alignas(64) SLIST_HEADER _blockPoolTop;
	SLIST_HEADER _emptyBlockTop;
	alignas(64) int _nodePerBlock;
	DWORD _tlsIndex;
	SLIST_HEADER _poolStates;

	PoolState& GetPoolStateRef()
	{
		PoolState* pPoolState = (PoolState*)TlsGetValue(_tlsIndex);
		if (pPoolState == nullptr)
		{
			pPoolState = (PoolState*)_aligned_malloc(sizeof(PoolState), 64);
			new (pPoolState) PoolState;
			TlsSetValue(_tlsIndex, pPoolState);
			InterlockedPushEntrySList(&_poolStates,pPoolState);
		}
		return *pPoolState;
	}

	void AllocBlock(PoolState& poolState)
	{
		NodeBlock* pBlock = (NodeBlock*)InterlockedPopEntrySList(&_blockPoolTop);
		if (pBlock == nullptr)
		{
			pBlock = (NodeBlock*)_aligned_malloc(sizeof(NodeBlock), MEMORY_ALLOCATION_ALIGNMENT);
			new (pBlock) NodeBlock(_nodePerBlock);
		}
		poolState.pTopNode = pBlock->pTopNode;
		poolState.remainCnt = _nodePerBlock;
		InterlockedPushEntrySList(&_emptyBlockTop, pBlock);
	}

	void FreeBlock(PoolState& poolState)
	{
		NodeBlock* pEmptyBlock = (NodeBlock*)InterlockedPopEntrySList(&_emptyBlockTop);
		pEmptyBlock->pTopNode = poolState.pFreePrev->pNext;
		poolState.pFreePrev->pNext = nullptr;
		poolState.remainCnt = _nodePerBlock;
		InterlockedPushEntrySList(&_blockPoolTop, pEmptyBlock);
	}
public:
	TlsObjectPool(int nodePerBlock = 512)
	{
		InitializeSListHead(&_blockPoolTop);
		InitializeSListHead(&_emptyBlockTop);
		InitializeSListHead(&_poolStates);
		_nodePerBlock = nodePerBlock;
		_tlsIndex = TlsAlloc();
	}
	~TlsObjectPool()
	{
		NodeBlock* pBlock = nullptr;
		PoolState* pPoolState = nullptr;
		while (pBlock = (NodeBlock*)InterlockedPopEntrySList(&_emptyBlockTop))
		{
			pBlock->~NodeBlock();
			_aligned_free(pBlock);
		}
		while (pBlock = (NodeBlock*)InterlockedPopEntrySList(&_blockPoolTop))
		{
			pBlock->~NodeBlock();
			_aligned_free(pBlock);
		}
		while (pPoolState =(PoolState*)InterlockedPopEntrySList(&_poolStates))
		{
			_aligned_free(pPoolState);
		}
		TlsFree(_tlsIndex);
	}

	T* Alloc()
	{
		PoolState& poolState = GetPoolStateRef();
		if (poolState.pTopNode == nullptr)
		{
			AllocBlock(poolState);
		}

		Node* pOldTop = poolState.pTopNode;
		poolState.pTopNode = pOldTop->pNext;
		poolState.remainCnt--;
		poolState.allocatingCnt++;
		if constexpr (_bPlacementNew)
		{
			new ((T*)pOldTop)T();
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
		PoolState& poolState = GetPoolStateRef();

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
			FreeBlock(poolState);
		}
	}
};