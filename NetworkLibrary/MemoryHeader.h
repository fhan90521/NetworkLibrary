#pragma once
struct MemoryHeader
{
	MemoryHeader* pNext = nullptr;
	size_t allocSize;
};