#include "WorkThreadPool.h"
#include "JobQueue.h"
#include "WorkType.h"
void WorkThreadPool::CreateNewCompletionPort(DWORD dwNumberOfConcurrentThreads)
{
    _hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL,0, dwNumberOfConcurrentThreads);
}

void WorkThreadPool::WorkFunc()
{
	while (1)
	{
		DWORD JobType = 0;
		JobQueue*  pJobQueue = nullptr;
		int error;
		OVERLAPPED* pOverlapped = nullptr;
		int retval = GetQueuedCompletionStatus(_hCompletionPort, &JobType, (PULONG_PTR)&pJobQueue, &pOverlapped, INFINITE);
		if (pOverlapped == nullptr|| retval == false)
		{
			Log::LogOnFile(Log::SYSTEM_LEVEL, "GQCS pOverlapped null error: %d\n", WSAGetLastError());
			DebugBreak();
		}
		else if(JobType == PROCESS_JOB)
		{
			//아래 순서 지켜져야 한다 refCnt를 생각해서 정해놓은 순서
			//process job하기 전에만 _selfPtr에 접근하는 스레드가 하나라는 걸 보장해서 실행전에 복사하고 nullptr로 바꾸어야 해야함
			SharedPtr<JobQueue> jobQ = pJobQueue->_selfPtr;
			jobQ->_selfPtr = nullptr;
			jobQ->ProcessJob();
		
		}
		else
		{
			break;
		}
	}
}

WorkThreadPool::WorkThreadPool(int concurrentThreadCnt, int workThreadCnt)
{
	CreateNewCompletionPort(concurrentThreadCnt);
	for (int i = 0; i < workThreadCnt; i++)
	{
		_threadList.push_back( new std::jthread(&WorkThreadPool::WorkFunc, this ) );
	}
}

WorkThreadPool::~WorkThreadPool()
{
	for (int i = 0; i < _threadList.size(); i++)
	{
		PostQueuedCompletionStatus(_hCompletionPort, SHUT_DOWN, SHUT_DOWN, (LPOVERLAPPED)SHUT_DOWN);
	}
	for (std::jthread* pThread : _threadList)
	{
		pThread->join();
		delete pThread;
	}
}

HANDLE WorkThreadPool::GetCompletionPortHandle()
{
	return _hCompletionPort;
}
