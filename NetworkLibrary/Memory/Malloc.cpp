#include "Malloc.h"
#include "CommonPool.h"
class GlobalCommonPool
{
public:
	CommonPool* pPool;
	GlobalCommonPool()
	{
		pPool = new CommonPool;
	}
	~GlobalCommonPool()
	{
		delete pPool;
	}
}globalCommonPool;

void* Malloc(int size)
{
	return globalCommonPool.pPool->Alloc(size);
}
void Free(void* ptr)
{
	globalCommonPool.pPool->Free(ptr);
}

