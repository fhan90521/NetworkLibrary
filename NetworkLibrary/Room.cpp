#include "Room.h"
#include "MyNew.h"
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
		if (_sessionsInRoom.find(sessionInfo.id) == _sessionsInRoom.end())
		{
			_sessionsInRoom.insert(sessionInfo.id);
			OnEnter(sessionInfo);
			_sessionCnt++;
		}
	}
	else if(retRequestEnter == ENTER_HOLD)
	{
		_tryEnterSessions.insert(sessionInfo.id);
	}
}

void Room::Leave(SessionInfo sessionInfo,int afterRoomID)
{
	_tryEnterSessions.erase(sessionInfo.id);
	auto iterSession = _sessionsInRoom.find(sessionInfo.id);
	if (iterSession != _sessionsInRoom.end())
	{
		OnLeave(sessionInfo);
		_sessionsInRoom.erase(iterSession);
		_sessionCnt--;
		_pRoomSystem->EnterRoom(sessionInfo,GetRoomID(),afterRoomID);
	}
	//_tryLeaveSessions.push_back(sessionInfo);
}
void Room::LeaveRoomSystem(SessionInfo sessionInfo)
{
	_tryEnterSessions.erase(sessionInfo.id);
	auto iterSession = _sessionsInRoom.find(sessionInfo.id);
	if (iterSession != _sessionsInRoom.end())
	{
		OnLeaveRoomSystem(sessionInfo);
		_sessionsInRoom.erase(iterSession);
		_sessionCnt--;
	}
}
void Room::UpdateJob()
{
	Update((_currentTime-_prevUpdateTime)/(1000.0));
	_prevUpdateTime = _currentTime;
	_bUpdating = false;
	InterlockedIncrement(&_updateCnt);
}
void Room::ChangeRoom(SessionInfo sessionInfo, int& beforeRoomID, int& afterRoomID)
{
	_pRoomSystem->ChangeRoom(sessionInfo, beforeRoomID, afterRoomID);
}
Room::~Room()
{
	
}

Room::Room(IOCPServer* pServer) : JobQueue(pServer)
{
	_prevUpdateTime = GetTickCount64();
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

