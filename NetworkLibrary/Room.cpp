#include "Room.h"
#include "MyNew.h"
#include "RoomManager.h"
void Room::ProcessEnter()
{
	for (auto itSessionInfo = _tryEnterSessions.begin(); itSessionInfo != _tryEnterSessions.end();)
	{
		int ret = RequestEnter(*itSessionInfo);
		if (ret == ENTER_HOLD)
		{
			itSessionInfo++;
		}
		else
		{
			if (ret == ENTER_SUCCESS)
			{
				OnEnter(*itSessionInfo);
			}
			itSessionInfo = _tryEnterSessions.erase(itSessionInfo);
		}
	}
}
void Room::TryEnter(SessionInfo sessionInfo)
{
	int retRequestEnter =RequestEnter(sessionInfo);
	if (retRequestEnter == ENTER_SUCCESS)
	{
		OnEnter(sessionInfo);
	}
	else if(retRequestEnter == ENTER_HOLD)
	{
		_tryEnterSessions.insert(sessionInfo.id);
	}
}

void Room::Leave(SessionInfo sessionInfo)
{
	_tryEnterSessions.erase(sessionInfo.id);
	OnLeave(sessionInfo);
	//_tryLeaveSessions.push_back(sessionInfo);
}
void Room::UpdateJob()
{
	_currentTime = GetTickCount64();
	Update();
	_prevUpdateTime = _currentTime;
	_bUpdating = false;
}
Room::~Room()
{
	RoomManager::GetInstance()->DeregisterRoom(this);
}
Room::Room(IOCPServer* pServer) : JobQueue(pServer)
{
	RoomManager::GetInstance()->RegisterRoom(this);
}
void Room::EnterRoom(SessionInfo sessionInfo)
{
	TryDoSync(&Room::TryEnter, sessionInfo);
}

void Room::LeaveRoom(SessionInfo sessionInfo)
{
	TryDoSync(&Room::Leave, sessionInfo);
}

int Room::GetUpdateCnt()
{
	int ret = _updateCnt;
	_updateCnt = 0;
	return ret;
}

