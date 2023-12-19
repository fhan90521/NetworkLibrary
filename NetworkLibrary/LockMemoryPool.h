#pragma once

//Alloc 함수 내부 
// int* pDataSize = (int*)malloc(sizeof(int) + unitSize);
// * pDataSize = unitSize;
// malloc이 사용가능한 메모리가 부족하여 nullptr를 반환하는 경우 무시
#pragma warning(disable: 6011)
#include <Windows.h>
#include<iostream>
#include<vector>
#include "GetPOOLID.h"
#define COOKIE_VALUE (short)0xAAAA
//#define ADD_CHECK
using namespace std;

//#define CHECK_MINUS_INPUT
class LockMemoryPool
{
private:
	struct ChunkHeader
	{
		unsigned int chunkSize;
		ChunkHeader* pNext;
	};
	struct ChunkTail
	{
#ifdef ADD_CHECK
		short cookie;
		unsigned short id;
#endif
	};
	struct BlockInfo
	{
		ChunkHeader* pFirstChunk;
		ChunkHeader* pLastChunk;
	};
	class MemoryPool
	{
	private:
		ChunkHeader* _pTop=nullptr;
		int _chunkSize;
		int _chunkPerBlock;
		SRWLOCK _lock;
		unsigned short _id;
		std::vector<void*> pMemoryBlockVector;

		
	public:
		~MemoryPool()
		{
			for (void* pMemoryBlock : pMemoryBlockVector)
			{
				free(pMemoryBlock);
			}
		}
		MemoryPool(int chunkSize, int chunkPerBlock) : _chunkSize(chunkSize), _chunkPerBlock(chunkPerBlock)
		{
			_id = GetPOOLID();
			InitializeSRWLock(&_lock);
		}
		MemoryPool(const MemoryPool& other) = delete;
		MemoryPool& operator=(const MemoryPool& other) = delete;

		BlockInfo AllocBlock()
		{
			void* pMemoryBlock = malloc(_chunkPerBlock * _chunkSize);
			pMemoryBlockVector.push_back(pMemoryBlock);
			ChunkHeader* pNewChunkHeader=nullptr;
			for (int i = 0; i < _chunkPerBlock; i++)
			{
				pNewChunkHeader = (ChunkHeader*)((char*)pMemoryBlock + (i * _chunkSize));
				pNewChunkHeader->chunkSize = _chunkSize;
				if (i == _chunkPerBlock - 1)
				{
					pNewChunkHeader->pNext = nullptr;
				}
				else
				{
					pNewChunkHeader->pNext = (ChunkHeader*)((char*)pNewChunkHeader + _chunkSize);
				}

#ifdef ADD_CHECK
				ChunkTail* pChunkTail = (ChunkTail*)((char*)pNewChunkHeader + _chunkSize - sizeof(ChunkTail));
				pChunkTail->cookie = COOKIE_VALUE;
				pChunkTail->id = _id;
#endif
			}
			BlockInfo blockInfo;
			blockInfo.pFirstChunk = (ChunkHeader*)pMemoryBlock;
			blockInfo.pLastChunk = (ChunkHeader*)pNewChunkHeader;
			return blockInfo;
		}
		void* Alloc()
		{
			BlockInfo blockInfo;
			ChunkHeader* retP=nullptr;
			AcquireSRWLockExclusive(&_lock);
			if (_pTop != nullptr)
			{
				retP = _pTop;
				_pTop = _pTop->pNext;
			}
			ReleaseSRWLockExclusive(&_lock);

			if (retP == nullptr)
			{
				blockInfo = AllocBlock();
				retP = blockInfo.pFirstChunk;
				AcquireSRWLockExclusive(&_lock);
				blockInfo.pLastChunk->pNext = _pTop;
				_pTop = blockInfo.pFirstChunk->pNext;
				ReleaseSRWLockExclusive(&_lock);
			}

			return retP+1;
		}

		void Free(ChunkHeader* pChunkHeader)
		{
			
#ifdef ADD_CHECK
			ChunkTail* pChunkTail = (ChunkTail*)((char*)pChunkHeader + _chunkSize - sizeof(ChunkTail));
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
			AcquireSRWLockExclusive(&_lock);
			pChunkHeader->pNext = _pTop;
			_pTop = pChunkHeader;
			ReleaseSRWLockExclusive(&_lock);
		}
	};

private:
	enum
	{
	
		MAX_CHUNK_SIZE = 4096, 
		CHUNK_PER_BLOCK = 64,
		POOL_CNT = (256/8) + (256/16) + (512/32) + (1024/64) + (2048/128)
	};
	vector<MemoryPool*> _pMemoryPoolVec;
	MemoryPool* _pPoolTable[MAX_CHUNK_SIZE+1];
	inline static LockMemoryPool* pInstance=nullptr;
	LockMemoryPool()
	{
		int poolSize = 0;
		int tableIndex = 0;
		for (poolSize = 0 + 8; poolSize <= 256; poolSize += 8)
		{
			MemoryPool* pPool = new MemoryPool(poolSize, CHUNK_PER_BLOCK);
			_pMemoryPoolVec.push_back(pPool);
			while (tableIndex <= poolSize)
			{
				_pPoolTable[tableIndex] = pPool;
				tableIndex++;
			}
		}

		for (poolSize = 256+16 ; poolSize <= 512; poolSize += 16)
		{
			MemoryPool* pPool = new MemoryPool(poolSize, CHUNK_PER_BLOCK);
			_pMemoryPoolVec.push_back(pPool);
			while (tableIndex <= poolSize)
			{
				_pPoolTable[tableIndex] = pPool;
				tableIndex++;
			}
		}

		for (poolSize = 512 + 32; poolSize <= 1024; poolSize += 32)
		{
			MemoryPool* pPool = new MemoryPool(poolSize, CHUNK_PER_BLOCK);
			_pMemoryPoolVec.push_back(pPool);
			while (tableIndex <= poolSize)
			{
				_pPoolTable[tableIndex] = pPool;
				tableIndex++;
			}
		}

		for (poolSize = 1024 + 64; poolSize <= 2048; poolSize += 64)
		{
			MemoryPool* pPool = new MemoryPool(poolSize, CHUNK_PER_BLOCK);
			_pMemoryPoolVec.push_back(pPool);
			while (tableIndex <= poolSize)
			{
				_pPoolTable[tableIndex] = pPool;
				tableIndex++;
			}
		}

		for (poolSize = 2048 + 128; poolSize <= 4096; poolSize += 128)
		{
			MemoryPool* pPool = new MemoryPool(poolSize, CHUNK_PER_BLOCK);
			_pMemoryPoolVec.push_back(pPool);
			while (tableIndex <= poolSize)
			{
				_pPoolTable[tableIndex] = pPool;
				tableIndex++;
			}
		}
	}
	~LockMemoryPool()
	{
		for (MemoryPool* pPool : _pMemoryPoolVec)
		{
			delete pPool; 
		}
	}
public:
	static LockMemoryPool* GetInstance()
	{
		if (pInstance == nullptr)
		{
			pInstance = new LockMemoryPool;
			atexit(Destroy);
		}
		return pInstance;
	}
	static void Destroy()
	{
		delete pInstance;
	}
	void* Alloc(int required_size)
	{
#ifdef CHECK_MINUS_INPUT
		if (required_size <= 0)
		{
			std::cout << "Alloc size lower than 0" << std::endl;
			DebugBreak();
			return nullptr;
		}
#endif


		int essentialSize = sizeof(ChunkHeader) + required_size + sizeof(ChunkTail);
		if (essentialSize > MAX_CHUNK_SIZE)
		{
			ChunkHeader* pChunkHeader = (ChunkHeader*)malloc(essentialSize);
			pChunkHeader->chunkSize = essentialSize;
			return (pChunkHeader + 1);
		}
		return _pPoolTable[essentialSize]->Alloc();
	}

	void Free(void* pData)
	{
		ChunkHeader* pChunkHeader = (ChunkHeader*)((char*)pData - sizeof(ChunkHeader));
		if (pChunkHeader->chunkSize > MAX_CHUNK_SIZE)
		{
			free(pChunkHeader);
		}
		else
		{
			_pPoolTable[pChunkHeader->chunkSize]->Free(pChunkHeader);
		}
		return ;
	}
};