#pragma once
struct MemoryHeader
{
	MemoryHeader* pNext = nullptr;
	size_t allocSize;
};
struct MemoryTail
{
#ifdef ADD_CHECK
	short cookie;
	unsigned short id;
#endif
};