#pragma once
#include <Windows.h>
#include <vector>
#include "GetPOOLID.h"
//#include <iostream>
#define	COOKIE_SIZE 4 
//#define CHECK_COOKIE

class MemoryPool
{
private:
	int _dataSize;
	int _chunkSize;
	int _chunkPerBlock;
	short _POOLID;

	std::vector<void*> pMemoryChunkVector;
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
			void* pNewMemoryChunk = ((char*)pMemoryBlockBegin + (i * _chunkSize));
			//std::cout << "pNewMemoryChunk : " << pNewMemoryChunk << std::endl;
			pMemoryChunkVector[i] = pNewMemoryChunk;
			//std::cout << *itFreeChunk << std::endl;
		}
		indexFreeChunk = _chunkPerBlock-1;
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
#ifdef CHECK_COOKIE
	MemoryPool(int dataSize, int chunkPerBlock)
		: _dataSize(dataSize), _chunkSize(sizeof(int) + dataSize + COOKIE_SIZE), _chunkPerBlock(chunkPerBlock)
	{
		_POOLID = GetPOOLID();
	}
#else
	MemoryPool(int dataSize,int chunkSize ,int chunkPerBlock)
		: _dataSize(dataSize), _chunkSize(chunkSize), _chunkPerBlock(chunkPerBlock)
	{
		_POOLID = GetPOOLID();
	}
#endif
	MemoryPool(const MemoryPool& other) = delete;
	MemoryPool& operator=(const MemoryPool& other) = delete;

	void* Alloc()
	{

		if (indexFreeChunk == -1)
		{
			AllocBlock();
		}
		*((int*)pMemoryChunkVector[indexFreeChunk]) = _dataSize;

#ifdef CHECK_COOKIE
		* ((short*)((char*)pMemoryChunkVector[indexFreeChunk] + sizeof(int) + _dataSize)) = _POOLID;
#endif

		void* retP = ((char*)pMemoryChunkVector[indexFreeChunk] + sizeof(int));
		indexFreeChunk--;

		//std::cout << "chunkSize: " << _dataSize << " blockNum: " << pMemoryBlockBeginList.size() << std::endl;
		return retP;
	}

	void Free(void* pData)
	{
#ifdef CHECK_COOKIE
		if (*(short*)((char*)pData + _dataSize) != _POOLID)
		{
			DebugBreak();
		}
#endif
		pMemoryChunkVector[++indexFreeChunk] = (void*)((char*)pData - sizeof(int));
	}
};

