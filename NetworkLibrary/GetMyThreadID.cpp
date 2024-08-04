#include "GetMyThreadID.h"
LONG newThreadID=-1;
thread_local alignas(64) LONG myThreadID=-1;
LONG GetMyThreadID()
{
	if (myThreadID == -1)
	{
		myThreadID = InterlockedIncrement(&newThreadID);
	}
	return myThreadID;
}