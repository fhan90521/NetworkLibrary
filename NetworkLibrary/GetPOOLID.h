#pragma once
inline unsigned short GetPOOLID()
{
	static unsigned short ObjectPoolNum = 1;
	return ObjectPoolNum++;
}