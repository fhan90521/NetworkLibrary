#pragma once
#pragma once
#include "LockFreeObjectPool.h"
#include "GetMyThreadID.h"
#include "GetPOOLID.h"
#include "MemoryHeader.h"
#include "MyWindow.h"
class TlsMemoryPool
{
private:
	enum
	{
		THREADCNT = 16
	};
	struct MemoryBlock :SLIST_ENTRY
	{
	private:
		void* allocPtr = nullptr;
	public:
		MemoryHeader* pTop = nullptr;
		MemoryBlock(int chunkSize, int chunkPerBlock)  
		{
			void* pBlock = malloc(chunkPerBlock * chunkSize);
			MemoryHeader* pMemoryHeader = nullptr;
			for (int i = 0; i < chunkPerBlock; i++)
			{
				pMemoryHeader = (MemoryHeader*)((char*)pBlock + (i * chunkSize));
				pMemoryHeader->allocSize = chunkSize;
				if (i == chunkPerBlock - 1)
				{
					pMemoryHeader->pNext = nullptr;
				}
				else
				{
					pMemoryHeader->pNext = (MemoryHeader*)((char*)pMemoryHeader + chunkSize);
				}
			}
			allocPtr = pBlock;
			pTop = (MemoryHeader*)pBlock;
		}
		~MemoryBlock()
		{
			free(allocPtr);
		}
	};

	struct alignas(64) PoolState
	{
		MemoryHeader* pTop = nullptr;
		MemoryHeader* pFreePrev = nullptr;
		int remainCnt = 0;
	};


	PoolState _poolStateArr[THREADCNT];
	LockFreeObjectPool<MemoryBlock, false> _blockPool;
	SLIST_HEADER _blockListHead;
	int _chunkSize;
	int _chunkPerBlock;

	void AllocBlock()
	{
		PoolState& poolState = _poolStateArr[GetMyThreadID()];
		MemoryBlock* pBlock = _blockPool.Alloc(_chunkSize, _chunkPerBlock);
		poolState.pTop = pBlock->pTop;
		poolState.remainCnt = _chunkPerBlock;
		InterlockedPushEntrySList(&_blockListHead, pBlock);
	}

	void FreeBlock()
	{
		PoolState& poolState = _poolStateArr[GetMyThreadID()];
		MemoryBlock* pOldTopBlock = (MemoryBlock*)InterlockedPopEntrySList(&_blockListHead);
		pOldTopBlock->pTop = poolState.pFreePrev->pNext;
		poolState.pFreePrev->pNext = nullptr;
		poolState.remainCnt = _chunkPerBlock;
		_blockPool.Free(pOldTopBlock);
	}
public:
	TlsMemoryPool(int chunkSize, int chunkPerBlock)
	{
		InitializeSListHead(&_blockListHead);
		_chunkSize = chunkSize;
		_chunkPerBlock = max(chunkPerBlock, 2);
	}

	~TlsMemoryPool()
	{
		MemoryBlock* pBlock;
		for (int i = 0; i < THREADCNT; i++)
		{
			while (pBlock = (MemoryBlock*)InterlockedPopEntrySList(&_blockListHead))
			{
				pBlock->pTop = nullptr;
				_blockPool.Free(pBlock);
			}
		}
	}

	void* Alloc(void)
	{
		PoolState& poolState = _poolStateArr[GetMyThreadID()];
		if (poolState.pTop == nullptr)
		{
			AllocBlock();
		}
		MemoryHeader* pOldTop = poolState.pTop;
		poolState.pTop = pOldTop->pNext;
		poolState.remainCnt--;
		return pOldTop+1;
	}

	void Free(MemoryHeader* pMemoryHeader)
	{
		PoolState& poolState = _poolStateArr[GetMyThreadID()];

		pMemoryHeader->pNext = poolState.pTop;
		poolState.pTop = pMemoryHeader;
		poolState.remainCnt++;

		if (poolState.remainCnt == _chunkPerBlock + 1)
		{
			poolState.pFreePrev = poolState.pTop;
		}
		if (poolState.remainCnt == _chunkPerBlock * 2)
		{
			FreeBlock();
		}
	}
};