#pragma once

// malloc이 사용가능한 메모리가 부족하여 nullptr를 반환하는 경우 무시
#pragma warning(disable: 6011)

#include "MyWindow.h"
#include<vector>
#include "TlsMemoryPool.h"
#include "MemoryHeader.h"

//#define CHECK_IMPOSSIBLE_INPUT
class CommonPool
{
	typedef TlsMemoryPool MemoryPool;
	enum: int
	{
		MAX_CHUNK_SIZE = 4096,
		POOL_CNT = (512 / 16) + (512 / 32) + (1024 / 64) + (2048 / 128)
	};
	std::vector<MemoryPool*> _pMemoryPoolVec;
	MemoryPool* _pPoolTable[MAX_CHUNK_SIZE + 1];
	int _chunkPerBlock;
public:
	CommonPool(int chunkPerBlock = 512)
	{
		_chunkPerBlock = chunkPerBlock;
		int poolSize = 0;
		int tableIndex = 0;

		for (poolSize = 0 + 16; poolSize <= 512; poolSize += 16)
		{
			MemoryPool* pPool = new MemoryPool(poolSize, _chunkPerBlock);
			_pMemoryPoolVec.push_back(pPool);
			while (tableIndex <= poolSize)
			{
				_pPoolTable[tableIndex] = pPool;
				tableIndex++;
			}
		}

		for (poolSize = 512 + 32; poolSize <= 1024; poolSize += 32)
		{
			MemoryPool* pPool = new MemoryPool(poolSize, _chunkPerBlock);
			_pMemoryPoolVec.push_back(pPool);
			while (tableIndex <= poolSize)
			{
				_pPoolTable[tableIndex] = pPool;
				tableIndex++;
			}
		}

		for (poolSize = 1024 + 64; poolSize <= 2048; poolSize += 64)
		{
			MemoryPool* pPool = new MemoryPool(poolSize, _chunkPerBlock);
			_pMemoryPoolVec.push_back(pPool);
			while (tableIndex <= poolSize)
			{
				_pPoolTable[tableIndex] = pPool;
				tableIndex++;
			}
		}

		for (poolSize = 2048 + 128; poolSize <= 4096; poolSize += 128)
		{
			MemoryPool* pPool = new MemoryPool(poolSize, _chunkPerBlock);
			_pMemoryPoolVec.push_back(pPool);
			while (tableIndex <= poolSize)
			{
				_pPoolTable[tableIndex] = pPool;
				tableIndex++;
			}
		}
	}
	~CommonPool()
	{
		for (MemoryPool* pPool : _pMemoryPoolVec)
		{
			delete pPool;
		}
	}
public:
	void* Alloc(size_t size)
	{
#ifdef CHECK_IMPOSSIBLE_INPUT
		if (size == 0)
		{
			Log::LogOnFile(Log::SYSTEM_LEVEL, "Alloc size lower than 0\n");
			DebugBreak();
			return nullptr;
		}
#endif
		size_t essentialSize = sizeof(MemoryHeader) + size + sizeof(MemoryTail);
		if (essentialSize > MAX_CHUNK_SIZE)
		{
			MemoryHeader* pMemoryHeader = (MemoryHeader*)malloc(essentialSize);
			pMemoryHeader->allocSize = essentialSize;
			return (pMemoryHeader + 1);
		}
		else
		{
			return _pPoolTable[essentialSize]->Alloc();
		}
	}

	void Free(void* pData)
	{
		MemoryHeader* pMemoryHeader = (MemoryHeader*)((char*)pData - sizeof(MemoryHeader));
		if (pMemoryHeader->allocSize > MAX_CHUNK_SIZE)
		{
			free(pMemoryHeader);
		}
		else
		{
			_pPoolTable[pMemoryHeader->allocSize]->Free(pMemoryHeader);
		}
		return;
	}
};