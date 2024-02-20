#include "Room.h"
#include "MyNew.h"
void Room::ProcessJob()
{
	int qSize = _jobQueue.Size();
	DWORD currentTime = timeGetTime();
	_deltaTime = (double)(currentTime - _lastProcessTime) / 1000;
	WorkerJob* pJob;
	for (int i = 0; i < qSize; i++)
	{
		_jobQueue.Dequeue(&pJob);
		pJob->Execute();
		Delete<WorkerJob>(pJob);
	}
	_lastProcessTime = timeGetTime();
	InterlockedExchange8(&_bProcessing, false);
	return;
}

void Room::MakeWorkerJob(CallbackType&& callback)
{
	_jobQueue.Enqueue(New<WorkerJob>(std::move(callback)));
}

template<typename T, typename Ret, typename... Args>
void Room::MakeWorkerJob(Ret(T::* memFunc)(Args...), Args... args)
{
	_jobQueue.Enqueue(New<CJob>(this, memFunc, std::forward<Args>(args)...));
}
Room::~Room()
{
	WorkerJob* pJob;
	while (_jobQueue.Dequeue(&pJob))
	{
		Delete<WorkerJob>(pJob);
	}
}
