#pragma once
#include "DBJob.h"
#include "LockFreeQueue.h"
#include "MyNew.h"
#include "MyWindow.h"
class DBJobQueue
{
private:
	friend class IOCPServer;
	typedef TlsObjectPool<DBJobQueue, false> QueuePool;
	friend class QueuePool;
	inline static QueuePool _DBJobQueuePool;
	IOCPServer* _pServer=nullptr;
	LockFreeQueue<DBJob*> _pDBJobQ;
	CHAR _bProcessing = false;
	DBJobQueue() {};
	~DBJobQueue()
	{
		DBJob* pJob;
		while (_pDBJobQ.Dequeue(&pJob))
		{
			Delete<DBJob>(pJob);
		}

	};
	bool GetProcessAuthority()
	{
		while (1)
		{
			if (_pDBJobQ.Size() == 0 || _bProcessing==true ||InterlockedExchange8(&_bProcessing, true) != false)
			{
				return false;
			}
			if (_pDBJobQ.Size() > 0)
			{
				return true;;
			}
			else
			{
				InterlockedExchange8(&_bProcessing, false);
			}
		}	
	}
	void ProcessDBJob()
	{
		DBJob* pJob = nullptr;
		for (int i = 0; i < _pDBJobQ.Size(); i++)
		{
			_pDBJobQ.Dequeue(&pJob);
			pJob->Execute(_pServer->GetDBConnection());
			Delete<DBJob>(pJob);
		}
		InterlockedExchange8(&_bProcessing, false);
		if (GetProcessAuthority() == true)
		{
			_pServer->PostDBJob(this);
		}
	}
public:
	
	template<typename T, typename... Args >
	void MakeDBJob(Args&&... args)
	{
		_pDBJobQ.Enqueue(New<T>(std::forward<Args>(args)...));
		if (GetProcessAuthority()==true)
		{
			_pServer->PostDBJob(this);
		}
	}

	int Size()
	{
		return _pDBJobQ.Size();
	}
};