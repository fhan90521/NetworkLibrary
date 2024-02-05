#pragma once
struct MemoryHeader
{
	MemoryHeader* pNext = nullptr;
	unsigned int allocSize;
};
struct MemoryTail
{
#ifdef ADD_CHECK
	short cookie;
	unsigned short id;
#endif
};