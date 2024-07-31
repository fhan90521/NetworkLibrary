#pragma once
#include "Job.h"
#include "LockQueue.h"
#include "MPSCQueue.h"
#include <memory>
#include "MyNew.h"
#include "IOCPServer.h"
#include "IOCPClient.h"
#include "MyStlContainer.h"
class JobQueue : public std::enable_shared_from_this<JobQueue>
{
private:
	friend class IOCPServer;
	friend class WorkThreadPool;
	HANDLE _hCompletionPort;
	SharedPtr<JobQueue> _selfPtr;
	MPSCQueue<Job*> _jobQueue;
	char _bProcessing = false;
	LONG _processedJobCnt = 0;
	void ProcessJob();
protected:
	ULONG64 _currentTime = 0;
	bool GetPopAuthority();
	virtual ~JobQueue();
	JobQueue(HANDLE hCompletionPort = NULL) :_hCompletionPort(hCompletionPort) {};
	void PostJob();
public:
	void TryDoSync(CallbackType&& callback)
	{
		_jobQueue.Enqueue(New<Job>(std::move(callback)));
		if (GetPopAuthority() == true)
		{
			ProcessJob();
		}
	}
	template<typename T, typename Ret, typename... Args>
	void TryDoSync(Ret(T::* memFunc)(Args...), Args... args)
	{
		_jobQueue.Enqueue(New<Job>((T*)this, memFunc, std::forward<Args>(args)...));
		if (GetPopAuthority() == true)
		{
			ProcessJob();
		}
	}
	void DoAsync(CallbackType&& callback)
	{
		_jobQueue.Enqueue(New<Job>(std::move(callback)));
		if (GetPopAuthority() == true)
		{
			PostJob();
		}
	}
	template<typename T, typename Ret, typename... Args>
	void DoAsync(Ret(T::* memFunc)(Args...), Args... args)
	{
		_jobQueue.Enqueue(New<Job>((T*)this, memFunc, std::forward<Args>(args)...));
		if (GetPopAuthority() == true)
		{
			PostJob();
		}
	}
	int GetProcessedJobCnt();
	int GetJobQueueLen();
	// Client¿ë
	template<typename T, typename Ret, typename... Args>
	void PushJob(Ret(T::* memFunc)(Args...), Args... args)
	{
		_jobQueue.Enqueue(New<Job>((T*)this, memFunc, std::forward<Args>(args)...));
	}
	bool PopJob(Job** pJob)
	{
		return _jobQueue.Dequeue(pJob);
	}
};