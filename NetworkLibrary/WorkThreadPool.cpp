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
			//����� �м��� quque�� pop�ؼ� ������ �Ҹ��� ȣ�� �Ŀ��� ��������� ���⸦ �ϱ� ������ ���۷����� ������ �ȵ�
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
