#pragma once
#include "GetMyThreadID.h"
#include "MemoryHeader.h"
#include "MyWindow.h"
#include <new.h>
class TlsMemoryPool
{
private:
	enum : int
	{
		THREADCNT =64
	};

	DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT)
	struct MemoryBlock :SLIST_ENTRY
	{
	private:
		void* allocPtr = nullptr;
	public:
		MemoryHeader* pTop = nullptr;
		MemoryBlock(size_t chunkSize, int chunkPerBlock)
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
	alignas(64) SLIST_HEADER _blockPoolTop;
	SLIST_HEADER _emptyBlockTop;
	alignas(64) size_t _chunkSize;
	int _chunkPerBlock;
	void AllocBlock()
	{
		PoolState& poolState = _poolStateArr[GetMyThreadID()];
		MemoryBlock* pBlock = (MemoryBlock*)InterlockedPopEntrySList(&_blockPoolTop);
		if (pBlock == nullptr)
		{
			pBlock = (MemoryBlock*)_aligned_malloc(sizeof(MemoryBlock), MEMORY_ALLOCATION_ALIGNMENT);
			new (pBlock) MemoryBlock(_chunkSize, _chunkPerBlock);
		}
		poolState.pTop = pBlock->pTop;
		poolState.remainCnt = _chunkPerBlock;
		InterlockedPushEntrySList(&_emptyBlockTop, pBlock);
	}

	void FreeBlock()
	{
		PoolState& poolState = _poolStateArr[GetMyThreadID()];
		MemoryBlock* pEmptyBlock = (MemoryBlock*)InterlockedPopEntrySList(&_emptyBlockTop);
		pEmptyBlock->pTop = poolState.pFreePrev->pNext;
		poolState.pFreePrev->pNext = nullptr;
		poolState.remainCnt = _chunkPerBlock;
		InterlockedPushEntrySList(&_blockPoolTop, pEmptyBlock);
	}
public:
	TlsMemoryPool(size_t chunkSize, int chunkPerBlock)
	{
		InitializeSListHead(&_blockPoolTop);
		InitializeSListHead(&_emptyBlockTop);
		_chunkSize = chunkSize;
		_chunkPerBlock = max(chunkPerBlock, 2);
	}

	~TlsMemoryPool()
	{
		MemoryBlock* pBlock=nullptr;
		while (pBlock = (MemoryBlock*)InterlockedPopEntrySList(&_emptyBlockTop))
		{
			pBlock->~MemoryBlock();
			_aligned_free(pBlock);
		}
		while (pBlock = (MemoryBlock*)InterlockedPopEntrySList(&_blockPoolTop))
		{
			pBlock->~MemoryBlock();
			_aligned_free(pBlock);
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