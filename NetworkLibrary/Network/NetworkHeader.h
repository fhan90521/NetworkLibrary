#pragma once
#include "OS/MyWindow.h"
#pragma pack(1)
struct WanHeader
{
	BYTE code;
	USHORT len;
	BYTE randKey;
	BYTE checkSum;
	inline static BYTE constKey = 0;
	inline static BYTE NetCode = 0;
	static void SetConstKey(BYTE key)
	{
		constKey = key;
	}
	static void SetNetCode(BYTE code)
	{
		NetCode = code;
	}
	BYTE DecodeCheckSum()
	{
		BYTE P = checkSum ^ (constKey + 1);
		checkSum = P ^ (randKey + 1);
		return P;
	}
	BYTE EncodeCheckSum()
	{
		BYTE P = checkSum ^ (randKey + 1);
		checkSum = P ^ (constKey + 1);
		return P;
	}
};
struct LanHeader
{
	USHORT len;
};
#pragma pack()