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
#define ATTACH_TAIL
using namespace std;

//#define CHECK_MINUS_INPUT
class CommonMemoryPool
{
private:
	struct chunkHeader
	{
		unsigned int chunkSize;
	};
	struct chunkTail
	{
		short cookie;
		unsigned short pool_id;
	};
	class MemoryPool
	{
	private:
		int _chunkSize;
		int _chunkPerBlock;
		unsigned short _pool_id;

		std::vector<chunkHeader*> pMemoryChunkVector;
		std::vector<void*> pMemoryBlockBeginVector;
		long long indexFreeChunk = -1;

		void AllocBlock()
		{

			void* pMemoryBlockBegin = malloc(_chunkPerBlock * _chunkSize);
			//std::cout << "pBegin: " << pMemoryBlockBegin << std::endl;
			pMemoryBlockBeginVector.push_back(pMemoryBlockBegin);
			pMemoryChunkVector.resize(pMemoryChunkVector.size() + _chunkPerBlock);

			for (int i = 0; i < _chunkPerBlock; i++)
			{
				chunkHeader* pNewMemoryChunk = (chunkHeader*)((char*)pMemoryBlockBegin + (i * _chunkSize));
				pNewMemoryChunk->chunkSize = _chunkSize;
#ifdef ATTACH_TAIL
				chunkTail* pChunkTail=(chunkTail*)((char*)pNewMemoryChunk +_chunkSize-sizeof(chunkTail));
				pChunkTail->cookie = COOKIE_VALUE;
				pChunkTail->pool_id = _pool_id;
#endif
				//std::cout << "pNewMemoryChunk : " << pNewMemoryChunk << std::endl;
				pMemoryChunkVector[i] = pNewMemoryChunk;
				//std::cout << *itFreeChunk << std::endl;
			}
			indexFreeChunk = _chunkPerBlock - 1;
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
			_pool_id = GetPOOLID();
		}
		MemoryPool(const MemoryPool& other) = delete;
		MemoryPool& operator=(const MemoryPool& other) = delete;

		void* Alloc()
		{

			if (indexFreeChunk == -1)
			{
				AllocBlock();
			}

			//std::cout << "chunkSize: " << _dataSize << " blockNum: " << pMemoryBlockBeginList.size() << std::endl;
			return ((char*)pMemoryChunkVector[indexFreeChunk--] + sizeof(chunkHeader));;
		}

		void Free(void* pData)
		{
#ifdef ATTACH_TAIL
			chunkTail* pChunkTail = (chunkTail*)((char*)pData-sizeof(chunkHeader) + _chunkSize - sizeof(chunkTail));
			if (pChunkTail->cookie != COOKIE_VALUE)
			{
				cout << "pool cookie modulation" << endl;
				DebugBreak();
			}
			else if (pChunkTail->pool_id != _pool_id)
			{
				cout << "pool id not match" << endl;
				DebugBreak();
			}
#endif
			pMemoryChunkVector[++indexFreeChunk] = (chunkHeader*)((char*)pData - sizeof(chunkHeader));
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

#ifdef ATTACH_TAIL
		int essentialSize = sizeof(chunkHeader) + required_size + sizeof(chunkTail);
#else
		int essentialSize = sizeof(chunkHeader) + required_size;
#endif
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
			pMemoryPoolArr[pChunkHeader->chunkSize/CHUNK_SIZE_UNIT -1]->Free(pData);
		}
		return ;
	}
};