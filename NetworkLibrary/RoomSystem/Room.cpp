#include "Room.h"
#include "RoomSystem.h"
#include "Network/IOCPServer.h"
#include "DebugTool/Log.h"
void Room::ProcessLeave()
{
	for (auto iter = _tryLeaveSessions.begin(); iter != _tryLeaveSessions.end();)
	{
		SessionInfo::ID sessionID = iter->first;
		int afterRoomID = iter->second;
		if (CheckCanLeave(sessionID)==false)
		{
			iter++;
		}
		else 
		{
			LeaveAndEnter(sessionID, afterRoomID);
			iter = _tryLeaveSessions.erase(iter);
		}
	}
}
void Room::Enter(SessionInfo sessionInfo)
{
	if (_sessionsInRoom.find(sessionInfo.Id()) == _sessionsInRoom.end())
	{
		_sessionsInRoom.insert(sessionInfo.Id());
		_pServer->ChangeRoomID(sessionInfo, _roomID);
		OnEnter(sessionInfo);
		_sessionCnt++;
	}
}
void Room::TryLeave(SessionInfo sessionInfo, int afterRoomID)
{
	auto iterSession = _sessionsInRoom.find(sessionInfo.Id());
	if (iterSession != _sessionsInRoom.end())
	{
		if (CheckCanLeave(sessionInfo) == true)
		{
			LeaveAndEnter(sessionInfo, afterRoomID);
		}
		else
		{
			_tryLeaveSessions[sessionInfo.Id()] = afterRoomID;
		}
	}
	else
	{
		Log::LogOnFile(Log::SYSTEM_LEVEL, "Room::TryLeave Error");
	}
}
void Room::LeaveAndEnter(SessionInfo sessionInfo,int afterRoomID)
{
	auto iterSession = _sessionsInRoom.find(sessionInfo.Id());
	if (iterSession != _sessionsInRoom.end())
	{
		OnLeave(sessionInfo);
		_sessionsInRoom.erase(iterSession);
		_sessionCnt--;
		_pRoomSystem->EnterRoom(sessionInfo, this, afterRoomID);
	}
}
void Room::UpdateJob()
{
	ProcessLeave();
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

