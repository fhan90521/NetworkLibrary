#pragma once
#include "OS/MyWindow.h"
#include "Container/MyStlContainer.h"
#include <thread>
class WorkThreadPool
{
private:
	HANDLE _hCompletionPort;
	void CreateNewCompletionPort(DWORD dwNumberOfConcurrentThreads);
	List<std::thread*> _threadList;
	void WorkFunc();
public:
	WorkThreadPool(int concurrentThreadCnt, int workThreadCnt);
	virtual ~WorkThreadPool();
	HANDLE GetCompletionPortHandle();
};