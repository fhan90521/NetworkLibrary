#pragma once
#include "WorkerJob.h"
#include "MpscQueue.h"
#include <memory>
#include "MyWindow.h"
#include "IOCPServer.h"
class Room
{
private:
	friend class IOCPServer;
	MpscQueue<WorkerJob*> _jobQueue;
	DWORD _lastProcessTime;
	bool _bReleased = false;
	char  _bProcessing = false;
	void ProcessJob();
protected:
	double _deltaTime;
	virtual ~Room();
	Room() { _lastProcessTime = timeGetTime(); };
	void operator delete (void* p)
	{
		free(p);
	}
public:
	void MakeWorkerJob(CallbackType&& callback);

	template<typename T, typename Ret, typename... Args>
	void MakeWorkerJob(Ret(T::* memFunc)(Args...), Args... args);
};