#pragma once

#ifdef _WIN64
	#define WORD_TYPE long long
	#define PWORD_TYPE long long*
#else
	#define WORD_TYPE int	
	#define PWORD_TYPE int*
#endif
void MemCopy(char* dest_pchar, char* src_pchar, int iSize);

//#define CUSTOM_MEMCOPY

#ifdef CUSTOM_MEMCOPY
	#define MEMCOPY MemCopy
#else
	#include <memory.h>
	#define MEMCOPY memcpy
#endif