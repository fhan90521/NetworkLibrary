#include "JobQueue.h"
JobQueue::~JobQueue()
{
	Job* pJob;
	while (_jobQueue.Dequeue(&pJob))
	{
		Delete<Job>(pJob);
	}	
}

int JobQueue::GetProcessedJobCnt()
{
	int ret = _processedJobCnt;
	InterlockedExchange(&_processedJobCnt, 0);
	return ret;
}

int JobQueue::GetJobQueueLen()
{
	return _jobQueue.Size();
}

void JobQueue::ProcessJob()
{
	Job* pJob = nullptr;
	int qSize = _jobQueue.Size();
	_currentTime = GetTickCount64();
	for (int i = 0; i < qSize; i++)
	{
		_jobQueue.Dequeue(&pJob);
		pJob->Execute();
		Delete<Job>(pJob);
	}
	InterlockedAdd(&_processedJobCnt, qSize);
	InterlockedExchange8(&_bProcessing, false);
	if (GetPopAuthority() == true)
	{
		_pServer->PostJob(this);
	}
}
bool JobQueue::GetPopAuthority()
{
	while (1)
	{
		if (_jobQueue.Size() == 0 || _bProcessing == true || InterlockedExchange8(&_bProcessing, true) != false)
		{
			break;
		}
		if (_jobQueue.Size() > 0)
		{
			return true;
		}
		else
		{
			InterlockedExchange8(&_bProcessing, false);
		}
	}
    return false;
}
