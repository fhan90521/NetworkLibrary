#include "GetPOOLID.h"
LONG GetPOOLID()
{
	static LONG newPOOLID = -1;
	return InterlockedIncrement(&newPOOLID);
}