#pragma once
#include "GetMyThreadID.h"
#include "MyStlContainer.h"
#include "MyWindow.h"
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

	
	Array<PoolState, MAX_THREAD_ID + 1> _poolStateArr;
	alignas(64) SLIST_HEADER _blockPoolTop;
	SLIST_HEADER _emptyBlockTop;
	alignas(64) int _nodePerBlock;
	void AllocBlock()
	{
		PoolState& poolState = _poolStateArr[GetMyThreadID()];
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

	void FreeBlock()
	{
		PoolState& poolState = _poolStateArr[GetMyThreadID()];
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
		_nodePerBlock = nodePerBlock;
	}
	~TlsObjectPool()
	{
		NodeBlock* pBlock = nullptr;
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
		for (int i = 0; i < _poolStateArr.size(); i++)
		{
			ret += _poolStateArr[i].allocatingCnt;
		}
		return ret;
	}
};