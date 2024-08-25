#pragma once
#include "GetMyThreadID.h"
#include "MemoryHeader.h"
#include "MyWindow.h"
#include <new.h>
#include "LockQueue.h"
#include "MyStlContainer.h"
class TlsMemoryPool
{
private:
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

	struct alignas(64) PoolState: SLIST_ENTRY
	{
		MemoryHeader* pTop = nullptr;
		MemoryHeader* pFreePrev = nullptr;
		int remainCnt = 0;
	};

	
	
	alignas(64) SLIST_HEADER _blockPoolTop;
	SLIST_HEADER _emptyBlockTop;
	alignas(64) size_t _chunkSize;
	int _chunkPerBlock;
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
			InterlockedPushEntrySList(&_poolStates, pPoolState);
		}
		return *pPoolState;
	}
	void AllocBlock(PoolState& poolState)
	{
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

	void FreeBlock(PoolState& poolState)
	{
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
		InitializeSListHead(&_poolStates);
		_chunkSize = chunkSize;
		_chunkPerBlock = max(chunkPerBlock, 2);
		_tlsIndex = TlsAlloc();
	}

	~TlsMemoryPool()
	{
		MemoryBlock* pBlock=nullptr;
		PoolState* pPoolState = nullptr;
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
		while (pPoolState = (PoolState*)InterlockedPopEntrySList(&_poolStates))
		{
			_aligned_free(pPoolState);
		}
		TlsFree(_tlsIndex);
	}

	void* Alloc(void)
	{
		PoolState& poolState = GetPoolStateRef();
		if (poolState.pTop == nullptr)
		{
			AllocBlock(poolState);
		}
		MemoryHeader* pOldTop = poolState.pTop;
		poolState.pTop = pOldTop->pNext;
		poolState.remainCnt--;
		return pOldTop+1;
	}

	void Free(MemoryHeader* pMemoryHeader)
	{
		PoolState& poolState = GetPoolStateRef();

		pMemoryHeader->pNext = poolState.pTop;
		poolState.pTop = pMemoryHeader;
		poolState.remainCnt++;

		if (poolState.remainCnt == _chunkPerBlock + 1)
		{
			poolState.pFreePrev = poolState.pTop;
		}
		if (poolState.remainCnt == _chunkPerBlock * 2)
		{
			FreeBlock(poolState);
		}
	}
};