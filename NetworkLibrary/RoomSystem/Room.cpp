#include "Room.h"
#include "RoomSystem.h"
void Room::ProcessEnter()
{
	for (auto itSessionInfo = _tryEnterSessions.begin(); itSessionInfo != _tryEnterSessions.end();)
	{
		int ret = RequestEnter(*itSessionInfo);
		if (ret == ENTER_HOLD)
		{
			itSessionInfo++;
		}
		else if (ret == ENTER_SUCCESS)
		{
			if (_sessionsInRoom.find(*itSessionInfo) == _sessionsInRoom.end())
			{
				_sessionsInRoom.insert(*itSessionInfo);
				OnEnter(*itSessionInfo);
			}
		}
		else
		{
			itSessionInfo = _tryEnterSessions.erase(itSessionInfo);
		}
	}
}
void Room::TryEnter(SessionInfo sessionInfo)
{
	int retRequestEnter =RequestEnter(sessionInfo);
	if (retRequestEnter == ENTER_SUCCESS)
	{
		if (_sessionsInRoom.find(sessionInfo.Id()) == _sessionsInRoom.end())
		{
			_sessionsInRoom.insert(sessionInfo.Id());
			OnEnter(sessionInfo);
			_sessionCnt++;
		}
	}
	else if(retRequestEnter == ENTER_HOLD)
	{
		_tryEnterSessions.insert(sessionInfo.Id());
	}
}

void Room::Leave(SessionInfo sessionInfo,int afterRoomID)
{
	auto iterSession = _sessionsInRoom.find(sessionInfo.Id());
	if (iterSession != _sessionsInRoom.end())
	{
		OnLeave(sessionInfo);
		_sessionsInRoom.erase(iterSession);
	}
	else
	{
		_tryEnterSessions.erase(sessionInfo.Id());
	}
	_sessionCnt--;
	_pRoomSystem->EnterRoom(sessionInfo, this, afterRoomID);
}
void Room::LeaveRoomSystem(SessionInfo sessionInfo)
{
	auto iterSession = _sessionsInRoom.find(sessionInfo.Id());
	if (iterSession != _sessionsInRoom.end())
	{
		OnLeaveRoomSystem(sessionInfo,true);
		_sessionsInRoom.erase(iterSession);
		
	}
	else
	{
		OnLeaveRoomSystem(sessionInfo, false);
		_tryEnterSessions.erase(sessionInfo.Id());
	}
	_sessionCnt--;
}
void Room::UpdateJob()
{
	Update((GetCurTime() - _prevUpdateTime) / (1000.0));
	_prevUpdateTime = GetCurTime();
	_bUpdating = false;
	InterlockedIncrement(&_updateCnt);
}
bool Room::ChangeRoom(SessionInfo sessionInfo, int afterRoomID)
{
	return _pRoomSystem->ChangeRoom(sessionInfo, this, afterRoomID);
}
Room::~Room()
{
	
}
Room::Room(HANDLE hCompletionPort) : JobQueue(hCompletionPort)
{
	_prevUpdateTime = GetTickCount64();
	_roomID = RoomSystem::INVALID_ROOM_ID;
}

int Room::GetUpdateCnt()
{
	int ret = _updateCnt;
	InterlockedExchange(&_updateCnt, 0);
	return ret;
}
int Room::GetSessionCnt()
{
	return _sessionCnt;
}
int Room::GetRoomID()
{
	return _roomID;
}

