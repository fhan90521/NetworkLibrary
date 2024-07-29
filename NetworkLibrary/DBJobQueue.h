#pragma once
#include "JobQueue.h"
#include <thread>
class DBJobQueue : private  JobQueue
{
private:
	enum
	{
		DBJobPushEvent = 0,
		ShutDownEvernt
	};
	HANDLE _eventArr[2];
	std::jthread _dbWorkThread;
	void DBWork();
public:
	DBJobQueue();
	virtual ~DBJobQueue();
	template<typename T, typename Ret, typename... Args>
	void PushDBJob(Ret(T::* memFunc)(Args...), Args... args)
	{
		_jobQueue.Enqueue(New<Job>((T*)this, memFunc, std::forward<Args>(args)...));
		SetEvent(_eventArr[DBJobPushEvent]);
	}
};