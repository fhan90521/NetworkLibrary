#include "DBJobQueue.h"

void DBJobQueue::DBWork()
{
	while (1)
	{
		DWORD ret = WaitForMultipleObjects(2, _eventArr,false, INFINITE);
		if (ret == WAIT_OBJECT_0)
		{
			Job* pJob = nullptr;
			int qSize = _jobQueue.Size();
			for (int i = 0; i < qSize; i++)
			{
				_jobQueue.Dequeue(&pJob);
				pJob->Execute();
				Delete<Job>(pJob);
			}
		}
		else
		{
			break;
		}
	}
}

DBJobQueue::DBJobQueue() :_dbWorkThread(&DBJobQueue::DBWork, this)
{
	_eventArr[DBJobPushEvent] = CreateEvent(NULL, false, false, NULL);
	_eventArr[ShutDownEvernt] = CreateEvent(NULL, false, false, NULL);
}

DBJobQueue::~DBJobQueue()
{
	SetEvent(_eventArr[ShutDownEvernt]);
	_dbWorkThread.join();
	CloseHandle(_eventArr[DBJobPushEvent]);
	CloseHandle(_eventArr[ShutDownEvernt]);
}
