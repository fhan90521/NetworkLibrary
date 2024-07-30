#pragma once
#include "MyWindow.h"
#include <thread>
#include "MyStlContainer.h"
class WorkThreadPool
{
private:
	HANDLE _hCompletionPort;
	void CreateNewCompletionPort(DWORD dwNumberOfConcurrentThreads);
	List<std::jthread*> _threadList;
	void WorkFunc();
public:
	WorkThreadPool(int concurrentThreadCnt, int workThreadCnt);
	virtual ~WorkThreadPool();
	HANDLE GetCompletionPortHandle();
};