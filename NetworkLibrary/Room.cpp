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
	if (_bProcessing == true)
	{
		return ;
	}
	if (InterlockedExchange8(&_bProcessing, true) != false)
	{
		return ;
	}

	while (1)
	{
		ProcessJob();
		if (_isUpdateTime)
		{
			Update(((float)(GetTickCount64() - _lastProcessTime)) / 1000);
			_lastProcessTime=GetTickCount64();
			_isUpdateTime = false;
		}
		if ((_jobQueue.Size()==0 &&  _isUpdateTime==false))
		{
			break;
		}
	}

	InterlockedExchange8(&_bProcessing, false);
	if (_jobQueue.Size() > 0 || _isUpdateTime == true)
	{
		if (_bProcessing == false)
		{
			_pServer->PqcsProcessRoom(this);
		}
	}

	if (_bClosed)
	{
		if (_pqcsCnt == 0)
		{
			_bCanRelease = true;
		}
	}
}

void Room::MakeRoomJob(CallbackType&& callback)
{
	_jobQueue.Enqueue(New<RoomJob>(std::move(callback)));
	ProcessRoom();
}

template<typename T, typename Ret, typename... Args>
void Room::MakeRoomJob(Ret(T::* memFunc)(Args...), Args... args)
{
	_jobQueue.Enqueue(New<WorkerJob>(this, memFunc, std::forward<Args>(args)...));
	ProcessRoom();
}
Room::~Room()
{
	RoomJob* pJob;
	while (_jobQueue.Dequeue(&pJob))
	{
		Delete<RoomJob>(pJob);
	}
}


