#pragma once
#include "Job.h"
#include "LockFreeQueue.h"
#include <memory>
#include "MyNew.h"
#include "IOCPServer.h"
#include "IOCPClient.h"
#include "MyStlContainer.h"
class JobQueue : public std::enable_shared_from_this<JobQueue>
{
private:
	friend class IOCPServer;
	friend class IOCPClient;
	IOCPServer* _pServer = nullptr;
	Queue<SharedPtr<JobQueue>> _selfPtrQueue;
	LockFreeQueue<Job*> _jobQueue;
	char _bProcessing = false;
	LONG _processedJobCnt = 0;
	void ProcessJob();
protected:
	ULONG64 _currentTime = 0;
	bool GetPopAuthority();
	virtual ~JobQueue();
	JobQueue(IOCPServer* pServer = nullptr) :_pServer(pServer) {};
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
			if (_pServer)
			{
				_pServer->PostJob(this);
			}
		}
	}
	template<typename T, typename Ret, typename... Args>
	void DoAsync(Ret(T::* memFunc)(Args...), Args... args)
	{
		_jobQueue.Enqueue(New<Job>((T*)this, memFunc, std::forward<Args>(args)...));
		if (GetPopAuthority() == true)
		{
			if (_pServer)
			{
				_pServer->PostJob(this);
			}
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
	void ProcJobOnClient()
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
	}
};