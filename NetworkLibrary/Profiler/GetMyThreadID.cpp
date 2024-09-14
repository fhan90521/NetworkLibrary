#include "GetMyThreadID.h"
#include "OS/MyWindow.h"
#include "DebugTool/Log.h"
long newThreadID=-1;
thread_local alignas(64) long myThreadID=-1;
long GetMyThreadID()
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