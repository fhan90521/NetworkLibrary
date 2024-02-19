#include "Room.h"
#include "MyNew.h"
void Room::ProcessJob()
{
	int qSize = _jobQueue.Size();
	CJob* pJob;
	for (int i = 0; i < qSize; i++)
	{
		_jobQueue.Dequeue(&pJob);
		pJob->Execute();
		Delete<CJob>(pJob);
	}
	InterlockedExchange8(&_bProcessing, false);
	return;
}

void Room::DoAsync(CallbackType&& callback)
{
	_jobQueue.Enqueue(New<CJob>(std::move(callback)));
}

template<typename T, typename Ret, typename... Args>
void Room::DoAsync(Ret(T::* memFunc)(Args...), Args... args)
{
	_jobQueue.Enqueue(New<CJob>(this, memFunc, std::forward<Args>(args)...));
}
Room::~Room()
{
	CJob* pJob;
	while (_jobQueue.Dequeue(&pJob))
	{
		Delete<CJob>(pJob);
	}
}
