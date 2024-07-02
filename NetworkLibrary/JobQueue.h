#pragma once
#include "Job.h"
#include "LockFreeQueue.h"
#include <memory>
#include "MyNew.h"
#include "IOCPServer.h"
#include "IOCPClient.h"
#include "MyStlContainer.h"
class JobQueue: public std::enable_shared_from_this<JobQueue>
{
private:
	friend class IOCPServer;
	friend class IOCPClient;
	IOCPServer* _pServer = nullptr;
	IOCPClient* _pClient = nullptr;
	Queue<SharedPtr<JobQueue>> _selfPtrQueue;
	LockFreeQueue<Job*> _jobQueue;
	char _bProcessing = false;
	LONG _processedJobCnt = 0;
	void ProcessJob();
protected:
	ULONG64 _currentTime = 0;
	bool GetPopAuthority();
	virtual ~JobQueue();
	JobQueue(IOCPServer* pServer) :_pServer(pServer) {};
	JobQueue(IOCPClient* pClient) :_pClient(pClient) {};
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
		if (GetPopAuthority()==true)
		{
			if (_pServer)
			{
				_pServer->PostJob(this);
			}
			else if (_pClient)
			{
				_pClient->PostJob(this);
			}
		}
	}
	template<typename T, typename Ret, typename... Args>
	void DoAsync(Ret(T::* memFunc)(Args...),Args... args)
	{
		_jobQueue.Enqueue(New<Job>((T*)this, memFunc, std::forward<Args>(args)...));
		if (GetPopAuthority() == true)
		{
			if (_pServer)
			{
				_pServer->PostJob(this);
			}
			else if (_pClient)
			{
				_pClient->PostJob(this);
			}
		}
	}
	int GetProcessedJobCnt();
	int GetJobQueueLen();
};