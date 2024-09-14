#include "JobQueue.h"
#include "WorkType.h"
#include "OS/MyWindow.h"
#include "DebugTool/Log.h"
JobQueue::~JobQueue()
{
	Job* pJob;
	while (_jobQueue.Dequeue(&pJob))
	{
		Delete<Job>(pJob);
	}	
}
void JobQueue::PostJob()
{
	_selfPtr = shared_from_this();
	//메모리 베리어가 필요 없을 것 같은 이유
	// 1. store는 순차적으로 일어난다 pqcs에 의해 변경된 queue의 상태를 읽은 경우 _selfPtr도 항상 store된 상태이다
	// 2. pqcs는 결국 멀티스레드를 지원하는 queue에 enqueue 하는 행위-> 멀티스레드에서의 동기화를 위해 내부적으로 interlocked함수나 동기화객체를 사용했을 것이다
	// MemoryBarrior 가 없어도 정상작동하나 pqcs 레퍼런스에는 pqcs가 memorybarrior역할을 한다는 얘기가 없으므로 일단 넣어둠.
	MemoryBarrier();
	bool ret = PostQueuedCompletionStatus(_hCompletionPort, PROCESS_JOB, (ULONG_PTR)this, (LPOVERLAPPED)PROCESS_JOB);
	if (ret == false)
	{
		Log::LogOnFile(Log::SYSTEM_LEVEL, "RequestJob error: %d\n", WSAGetLastError());
	}
}
int JobQueue::GetProcessedJobCnt()
{
	int ret = _processedJobCnt;
	InterlockedExchange(&_processedJobCnt, 0);
	return ret;
}

size_t JobQueue::GetJobQueueLen()
{
	return _jobQueue.Size();
}

void JobQueue::ProcessJob()
{
	Job* pJob = nullptr;
	size_t qSize = _jobQueue.Size();
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
		PostJob();
	}
}
bool JobQueue::GetPopAuthority()
{
	while (1)
	{
		//Size체크가 인터락함수보다 먼저 일어나야함
		if (_jobQueue.Size() ==0 || InterlockedExchange8(&_bProcessing, true) != false)
		{
			return false;
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
}
