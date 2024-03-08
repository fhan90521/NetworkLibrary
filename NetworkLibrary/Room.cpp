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
	ULONG64 currentTime = GetTickCount64();
	if (_jobQueue.Size() == 0 && currentTime - _prevUpdateTime < _updatePeriod)
	{
		return;
	}
	if (InterlockedExchange8(&_bProcessing, true) != false)
	{
		return ;
	}
	_currentTime = currentTime;
	ProcessJob();
	if (_currentTime - _prevUpdateTime < _updatePeriod)
	{
		ProcessEnter();
		ProcessLeave();
		Update();
		_prevUpdateTime = _currentTime;
	}
	_bProcessing = false;

}

void Room::ProcessEnter()
{
	for (auto itSessionInfo = _tryEnterSessions.begin(); itSessionInfo != _tryEnterSessions.end();)
	{
		int ret = RequestEnter(*itSessionInfo);
		if (ret == 0)
		{
			itSessionInfo++;
		}
		else
		{
			if (ret == 1)
			{
				OnEnter(*itSessionInfo);
			}
			itSessionInfo = _tryEnterSessions.erase(itSessionInfo);
		}
	}
}

void Room::ProcessLeave()
{
	for (auto itSessionInfo = _tryLeaveSessions.begin(); itSessionInfo != _tryLeaveSessions.end();)
	{
		bool ret = RequestLeave(*itSessionInfo);
		if (ret == false)
		{
			itSessionInfo++;
		}
		else
		{	
			OnLeave(*itSessionInfo);
			itSessionInfo = _tryEnterSessions.erase(itSessionInfo);
		}
	}
}


void Room::TryEnter(SessionInfo sessionInfo)
{
	_tryEnterSessions.push_back(sessionInfo);
}

void Room::TryLeave(SessionInfo sessionInfo)
{
	_tryLeaveSessions.push_back(sessionInfo);
}
void Room::EnterRoom(SessionInfo sessionInfo)
{
	MakeRoomJob(&Room::TryEnter, sessionInfo);
}

void Room::LeaveRoom(SessionInfo sessionInfo)
{
	MakeRoomJob(&Room::TryLeave, sessionInfo);
}

void Room::MakeRoomJob(CallbackType&& callback)
{
	_jobQueue.Enqueue(New<RoomJob>(std::move(callback)));
	ProcessRoom();
}

template<typename T, typename Ret, typename... Args>
void Room::MakeRoomJob(Ret(T::* memFunc)(Args...), Args... args)
{
	_jobQueue.Enqueue(New<RoomJob>(this, memFunc, std::forward<Args>(args)...));
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


