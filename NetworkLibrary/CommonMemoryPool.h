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
#define ADD_CHECK
using namespace std;

//#define CHECK_MINUS_INPUT
class CommonMemoryPool
{
private:
	struct chunkHeader
	{
		unsigned int chunkSize;
		chunkHeader* pNext;
	};
	struct chunkTail
	{
#ifdef ADD_CHECK
		short cookie;
		unsigned short id;
#endif
	};
	class MemoryPool
	{
	private:
		chunkHeader* _pFreeChunk=nullptr;
		int _chunkSize;
		int _chunkPerBlock;
		unsigned short _id;
		std::vector<void*> pMemoryBlockBeginVector;

		void AllocBlock()
		{
			void* pMemoryBlockBegin = malloc(_chunkPerBlock * _chunkSize);
			pMemoryBlockBeginVector.push_back(pMemoryBlockBegin);
			chunkHeader* pNewChunkHeader;
			for (int i = 0; i < _chunkPerBlock; i++)
			{
				pNewChunkHeader = (chunkHeader*)((char*)pMemoryBlockBegin + (i * _chunkSize));
				pNewChunkHeader->chunkSize = _chunkSize;
				if (i == _chunkPerBlock - 1)
				{
					pNewChunkHeader->pNext = nullptr;
				}
				else
				{
					pNewChunkHeader->pNext = pNewChunkHeader + 1;
				}

#ifdef ADD_CHECK
				chunkTail* pChunkTail=(chunkTail*)((char*)pNewChunkHeader +_chunkSize-sizeof(chunkTail));
				pChunkTail->cookie = COOKIE_VALUE;
				pChunkTail->id = _id;
#endif
			}
			_pFreeChunk = (chunkHeader*) pMemoryBlockBegin;
		}
		void Clear()
		{
			for (void* pMemoryBlockBegin : pMemoryBlockBeginVector)
			{
				free(pMemoryBlockBegin);
			}
		}
	public:
		~MemoryPool()
		{
			Clear();
		}
		MemoryPool(int chunkSize, int chunkPerBlock) : _chunkSize(chunkSize), _chunkPerBlock(chunkPerBlock)
		{
			_id = GetPOOLID();
		}
		MemoryPool(const MemoryPool& other) = delete;
		MemoryPool& operator=(const MemoryPool& other) = delete;

		void* Alloc()
		{

			if (_pFreeChunk == nullptr)
			{
				AllocBlock();
			}
			void* retP = _pFreeChunk;
			_pFreeChunk = _pFreeChunk->pNext;
			return retP;
		}

		void Free(chunkHeader* pChunkHeader)
		{
			chunkTail* pChunkTail = (chunkTail*)((char*)pChunkHeader + _chunkSize - sizeof(chunkTail));
#ifdef ADD_CHECK
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
			pChunkHeader->pNext = _pFreeChunk;
			_pFreeChunk = pChunkHeader;
		}
	};

private:
	enum
	{
		CHUNK_SIZE_UNIT = 16,
		UNIT_LIMIT= CHUNK_SIZE_UNIT*256,
		POOL_NUM= UNIT_LIMIT / CHUNK_SIZE_UNIT,
		CHUNK_PER_BLOCK = 10
	};
	MemoryPool** pMemoryPoolArr;

	inline static CommonMemoryPool* pInstance=nullptr;
	CommonMemoryPool()
	{
		pMemoryPoolArr = new MemoryPool * [POOL_NUM];
		for (int i = 0; i < POOL_NUM; i++)
		{
			pMemoryPoolArr[i] = nullptr;
		}
	}
	~CommonMemoryPool()
	{
		for (int i = 0; i < POOL_NUM; i++)
		{
			delete pMemoryPoolArr[i];
		}
		delete pMemoryPoolArr;
	}
public:
	static CommonMemoryPool* GetInstance()
	{
		if (pInstance == nullptr)
		{
			pInstance = new CommonMemoryPool;
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


		int essentialSize = sizeof(chunkHeader) + required_size + sizeof(chunkTail);
		if (essentialSize > UNIT_LIMIT)
		{
			chunkHeader* pChunkHeader = (chunkHeader*)malloc(essentialSize);
			pChunkHeader->chunkSize = essentialSize;
			return (pChunkHeader + 1);
		}
		

		int POOLindex;
		if (essentialSize % CHUNK_SIZE_UNIT == 0)
		{
			POOLindex = essentialSize / CHUNK_SIZE_UNIT -1;
		}
		else
		{
			POOLindex = (essentialSize / CHUNK_SIZE_UNIT);
		}

		if (pMemoryPoolArr[POOLindex] == nullptr)
		{
			pMemoryPoolArr[POOLindex] = new MemoryPool((POOLindex+1)* CHUNK_SIZE_UNIT, CHUNK_PER_BLOCK);
		}
		return pMemoryPoolArr[POOLindex]->Alloc();
	}

	void Free(void* pData)
	{
		chunkHeader* pChunkHeader = (chunkHeader*)((char*)pData - sizeof(chunkHeader));
		if (pChunkHeader->chunkSize > UNIT_LIMIT)
		{
			free(pChunkHeader);
		}
		else
		{
			pMemoryPoolArr[pChunkHeader->chunkSize/CHUNK_SIZE_UNIT -1]->Free(pChunkHeader);
		}
		return ;
	}
};