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
			//어셈블리 분석상 quque는 pop해서 원소의 소멸자 호출 후에도 멤버변수에 쓰기를 하기 때문에 레퍼런스로 받으면 안됨
			SharedPtr<JobQueue> jobQueue = pJobQueue->_selfPtrQueue.front();
			pJobQueue->ProcessJob();
			pJobQueue->_selfPtrQueue.pop();
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
