#include "Room.h"
#include "MyNew.h"
#include "MyWindow.h"
void Room::ProcessJob()
{
	int qSize = _jobQueue.Size();
	RoomJob* pJob;
	for (int i = 0; i < qSize; i++)
	{
		_jobQueue.Dequeue(&pJob);
		pJob->Execute();
		Delete<RoomJob>(pJob);
	}
	return;
}
void Room::ProcessRoom()
{
	ProcessJob();
	if (_isUpdateTime)
	{
		Update(((float)(GetTickCount64() - _lastProcessTime)) / 1000);
		_lastProcessTime=GetTickCount64();
		_isUpdateTime = false;
	}
	_bProcessing = false;
}

void Room::MakeRoomJob(CallbackType&& callback)
{
	_jobQueue.Enqueue(New<RoomJob>(std::move(callback)));
}

template<typename T, typename Ret, typename... Args>
void Room::MakeRoomJob(Ret(T::* memFunc)(Args...), Args... args)
{
	_jobQueue.Enqueue(New<RoomJob>(this, memFunc, std::forward<Args>(args)...));
}

Room::~Room()
{
	RoomJob* pJob;
	while (_jobQueue.Dequeue(&pJob))
	{
		Delete<RoomJob>(pJob);
	}
}


