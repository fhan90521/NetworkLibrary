#include "GetMyThreadID.h"
#include "Log.h"
LONG newThreadID=-1;
thread_local alignas(64) LONG myThreadID=-1;
LONG GetMyThreadID()
{
	if (myThreadID == -1)
	{
		myThreadID = InterlockedIncrement(&newThreadID);
		if (myThreadID > MAX_THREAD_ID)
		{
			Log::LogOnFile(Log::SYSTEM_LEVEL, "over Max thread id error");
			DebugBreak();
		}
	}
	return myThreadID;
}