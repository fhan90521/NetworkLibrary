#pragma once
#include "MyWindow.h"
#include <vector>
#include "GetPOOLID.h"
#include "LockFreeStack.h"
#include"MemoryHeader.h"
class LockFreeMemoryPool
{
private:
	union Top
	{
		struct BitPartial
		{
			LONG64 pChunkHeader : 47;
			LONG64 cnt : 17;
		}bitPartial;
		LONG64 entire;
	};
private:
	alignas(64) Top _top;
	LockFreeStack<void*> _allocPtrStack;
	int _chunkSize;
	int _chunkPerBlock;
	LONG _id;
private:
	MemoryHeader* AllocBlock()
	{
		void* pMemoryBlock = malloc(_chunkPerBlock * _chunkSize);
		MemoryHeader* pNewChunkHeader = nullptr;
		for (int i = 0; i < _chunkPerBlock; i++)
		{
			pNewChunkHeader = (MemoryHeader*)((char*)pMemoryBlock + (i * _chunkSize));
			pNewChunkHeader->allocSize = _chunkSize;
			if (i == _chunkPerBlock - 1)
			{
				pNewChunkHeader->pNext = nullptr;
			}
			else
			{
				pNewChunkHeader->pNext = (MemoryHeader*)((char*)pNewChunkHeader + _chunkSize);
			}

#ifdef ADD_CHECK
			MemoryTail* pChunkTail = (MemoryTail*)((char*)pNewChunkHeader + _chunkSize - sizeof(ChunkTail));
			pChunkTail->cookie = COOKIE_VALUE;
			pChunkTail->id = _id;
#endif
		}
		MemoryHeader* pLastHeader = pNewChunkHeader;
		Top curTop;
		Top newTop;
		newTop.entire = (LONG64)(((MemoryHeader*)pMemoryBlock)->pNext);
		while (1)
		{
			curTop = _top;
			pLastHeader->pNext = (MemoryHeader*)curTop.bitPartial.pChunkHeader;
			newTop.bitPartial.cnt = curTop.bitPartial.cnt + 1;
			if (InterlockedCompareExchange64(&_top.entire, newTop.entire, curTop.entire) == curTop.entire)
			{
				break;
			}
		}
		_allocPtrStack.Push(pMemoryBlock);
		return (MemoryHeader*)pMemoryBlock;
	}

public:
	~LockFreeMemoryPool()
	{
		void* allocPtr = nullptr;
		while (_allocPtrStack.Pop(&allocPtr))
		{
			free(allocPtr);
		}
	}
	LockFreeMemoryPool(int chunkSize, int chunkPerBlock)
	{
		_chunkSize = chunkSize;
		_chunkPerBlock = max(chunkPerBlock, 2);
		_id = GetPOOLID();
		_top.entire = NULL;
	}

	LockFreeMemoryPool(const LockFreeMemoryPool& other) = delete;
	LockFreeMemoryPool& operator=(const LockFreeMemoryPool& other) = delete;

	void* Alloc()
	{
		MemoryHeader* retP = nullptr;
		Top curTop;
		Top newTop;
		while (1)
		{
			curTop = _top;
			if (curTop.bitPartial.pChunkHeader == NULL)
			{
				retP = AllocBlock();
				break;
			}
			newTop.entire = (LONG64)((MemoryHeader*)(curTop.bitPartial.pChunkHeader))->pNext;
			newTop.bitPartial.cnt = curTop.bitPartial.cnt;
			if (InterlockedCompareExchange64(&_top.entire, newTop.entire, curTop.entire) == curTop.entire)
			{
				retP = (MemoryHeader*)curTop.bitPartial.pChunkHeader;
				break;
			}
		}
		return retP + 1;
	}
	void Free(MemoryHeader* pChunkHeader)
	{

#ifdef ADD_CHECK
		MemoryTail* pChunkTail = (MemoryTail*)((char*)pChunkHeader + _chunkSize - sizeof(ChunkTail));
		if (pChunkTail->cookie != COOKIE_VALUE)
		{
			cout << "object pool cookie modulation" << endl;
			DebugBreak();
		}
		else if (pChunkTail->id != _id)
		{
			cout << "pool id not match" << endl;
			DebugBreak();
		}
#endif
		Top curTop;
		Top newTop;
		newTop.bitPartial.pChunkHeader = (LONG64)pChunkHeader;
		while (1)
		{
			curTop = _top;
			pChunkHeader->pNext = (MemoryHeader*)curTop.bitPartial.pChunkHeader;
			newTop.bitPartial.cnt = curTop.bitPartial.cnt + 1;
			if (InterlockedCompareExchange64(&_top.entire, newTop.entire, curTop.entire) == curTop.entire)
			{
				break;
			}
		}
	}
};



