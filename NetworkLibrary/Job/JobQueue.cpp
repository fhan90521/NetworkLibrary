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
	//�޸� ����� �ʿ� ���� �� ���� ����
	// 1. store�� ���������� �Ͼ�� pqcs�� ���� ����� queue�� ���¸� ���� ��� _selfPtr�� �׻� store�� �����̴�
	// 2. pqcs�� �ᱹ ��Ƽ�����带 �����ϴ� queue�� enqueue �ϴ� ����-> ��Ƽ�����忡���� ����ȭ�� ���� ���������� interlocked�Լ��� ����ȭ��ü�� ������� ���̴�
	// MemoryBarrior �� ��� �����۵��ϳ� pqcs ���۷������� pqcs�� memorybarrior������ �Ѵٴ� ��Ⱑ �����Ƿ� �ϴ� �־��.
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
		//Sizeüũ�� ���Ͷ��Լ����� ���� �Ͼ����
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