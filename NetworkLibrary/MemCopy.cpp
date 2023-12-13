#include "MemCopy.h"
void MemCopy(char* dest_pchar, char* src_pchar, int iSize)
{
	PWORD_TYPE dest = (PWORD_TYPE)dest_pchar;
	PWORD_TYPE src = (PWORD_TYPE)src_pchar;

	int roop_cnt = iSize / sizeof(WORD_TYPE);
	int remain = iSize % sizeof(WORD_TYPE);

	for (int i = 0; i < roop_cnt; i++)
	{
		*dest = *src;
		dest++;
		src++;
	}

	char* dest_remain = (char*)dest;
	char* src_remain = (char*)src;

	for (int i = 0; i < remain; i++)
	{
		*dest_remain = *src_remain;
		dest_remain++;
		src_remain++;
	}

}