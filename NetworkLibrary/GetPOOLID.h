#pragma once
inline short GetPOOLID()
{
	static short ObjectPoolNum = 1;
	return ObjectPoolNum++;
}