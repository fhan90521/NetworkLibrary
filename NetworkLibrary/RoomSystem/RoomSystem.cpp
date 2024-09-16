#include "RoomSystem.h"
#include "Network/IOCPServer.h"
#include "DebugTool/Log.h"
bool RoomSystem::RegisterRoom(const SharedPtr<Room>& pRoom)
{
	if (pRoom->_roomID != RoomSystem::INVALID_ROOM_ID)
	{
		return false;
	}
	pRoom->_pRoomSystem = this;
	{
		SRWLockGuard<LOCK_TYPE::EXCLUSIVE> srwLockGuard(_roomsLock);
		int newRoomID = _validRoomIDs.top();
		_validRoomIDs.pop();
		pRoom->_roomID = newRoomID;
		_rooms[newRoomID] = pRoom;
	}
	return true;
}
void RoomSystem::CloseRoomSystem()
{
	bShutDown = true;
	_roomUpdateThread->join();
	Delete<std::thread>(_roomUpdateThread);
}
void RoomSystem::DeregisterRoom(const SharedPtr<Room>& pRoom)
{
	SRWLockGuard<LOCK_TYPE::EXCLUSIVE> srwLockGuard(_roomsLock);
	size_t eraseCnt = _rooms.erase(pRoom->_roomID);
	if (eraseCnt == 1)
	{
		_validRoomIDs.push(pRoom->_roomID);
	}
}
void RoomSystem::UpdateRooms()
{
	while (bShutDown==false)
	{
		ProcessLeaveSystem();
		{
			SRWLockGuard<LOCK_TYPE::SHARED> srwLockGuard(_roomsLock);
			ULONG64 currentTime = GetTickCount64();
			for (auto&  temp: _rooms)
			{
				SharedPtr<Room>& pRoom = temp.second;
				if (pRoom->_bUpdating == false)
				{
					ULONG64 timeDiff = currentTime - pRoom->_prevUpdateTime;
					if (timeDiff >= _updatePeriod)
					{
						pRoom->_bUpdating = true;
						pRoom->DoAsync(&Room::UpdateJob);
					}
				}
			}
		}
		Sleep(_updatePeriod / 3);
	}
}
RoomSystem::RoomSystem(IOCPServer* pServer)
{
	InitializeSRWLock(&_roomsLock);
	InitializeSRWLock(&_sessionsLock);
	InitializeSRWLock(&_leaveLock);
	_roomUpdateThread = New<std::thread>(&RoomSystem::UpdateRooms, this);
	for (int i = MAX_ROOM_ID; i >= 0; i--)
	{
		_validRoomIDs.push(i);
	}
	_pServer = pServer;
}
RoomSystem::~RoomSystem()
{
}
int RoomSystem::GetSessionCntInRoomSystem()
{
	return _sessions.size();
}

bool RoomSystem::EnterRoomSystem(SessionInfo sessionInfo, int roomID)
{

	//��� �뿡�� �������� �ʰ� ó�� �뿡 ����
	bool ret = false;
	SRWLockGuard<LOCK_TYPE::EXCLUSIVE> srwLockGuard(_sessionsLock);
	auto sessionToRoomIDIter = _sessions.find(sessionInfo.Id());
	if (sessionToRoomIDIter == _sessions.end())
	{
		SRWLockGuard<LOCK_TYPE::SHARED> srwLockGuard(_roomsLock);
		auto roomIter = _rooms.find(roomID);
		if (roomIter != _rooms.end())
		{
			ret = true;
			_sessions[sessionInfo.Id()] = roomID;
			roomIter->second->DoAsync(&Room::Enter, sessionInfo);
			_pServer->ChangeRoomID(sessionInfo, roomID);
		}
	}
	return ret;
}
bool RoomSystem::ChangeRoom(SessionInfo sessionInfo, Room* beforeRoom, int afterRoomID)
{
	bool ret = false;
	{
		SRWLockGuard<LOCK_TYPE::EXCLUSIVE> srwLockGuard(_sessionsLock);
		auto sessionToRoomIDIter = _sessions.find(sessionInfo.Id());
		if (sessionToRoomIDIter != _sessions.end())
		{
			if (sessionToRoomIDIter->second == beforeRoom->GetRoomID())
			{
				sessionToRoomIDIter->second = CHANGING_ROOM_ID;
				_pServer->ChangeRoomID(sessionInfo, CHANGING_ROOM_ID);
				beforeRoom->DoAsync(&Room::TryLeave, sessionInfo, afterRoomID);
				ret = true;
			}
		}
	}
	return ret;
}
void RoomSystem::EnterRoom(SessionInfo sessionInfo, Room* beforeRoom, int afterRoomID)
{
	{
		SRWLockGuard<LOCK_TYPE::EXCLUSIVE> srwLockGuard(_sessionsLock);
		auto sessionToRoomIDIter = _sessions.find(sessionInfo.Id());
		if (sessionToRoomIDIter != _sessions.end())
		{
			if (sessionToRoomIDIter->second == LEAVE_ROOM_SYSTEM)
			{
				RegisterLeaveSession(sessionInfo);
				_sessions.erase(sessionToRoomIDIter);
			}
			else if (sessionToRoomIDIter->second == CHANGING_ROOM_ID)
			{
				SRWLockGuard<LOCK_TYPE::SHARED> srwLockGuard(_roomsLock);
				auto afterRoomIter = _rooms.find(afterRoomID);
				if (afterRoomIter != _rooms.end())
				{
					sessionToRoomIDIter->second = afterRoomID;
					afterRoomIter->second->DoAsync(&Room::Enter, sessionInfo);
					_pServer->ChangeRoomID(sessionInfo, afterRoomID);
					return;
				}
				else
				{
					beforeRoom->DoAsync(&Room::Enter, sessionInfo);
					return;
				}
			}
			else
			{
				//������ �Ұ����� ���
				Log::LogOnFile(Log::SYSTEM_LEVEL, "EnterRoom Impossible Error 1");
			}
		}
		else
		{
			//������ �Ұ����� ���
			Log::LogOnFile(Log::SYSTEM_LEVEL, "EnterRoom Impossible Error 2");
		}
	}
}
void RoomSystem::RegisterLeaveSession(SessionInfo sessionInfo)
{
	SRWLockGuard<LOCK_TYPE::EXCLUSIVE> srwLockGuard(_leaveLock);
	_tryLeaveSessions.insert(sessionInfo.Id());
}
void RoomSystem::ProcessLeaveSystem()
{
	SRWLockGuard<LOCK_TYPE::EXCLUSIVE> srwLockGuard(_leaveLock);
	for (auto iter = _tryLeaveSessions.begin(); iter != _tryLeaveSessions.end();)
	{
		SessionInfo::ID sessionID = *iter;
		if (CheckCanLeaveSystem(sessionID) == false)
		{
			iter++;
		}
		else
		{
			OnLeaveRoomSystem(sessionID);
			iter = _tryLeaveSessions.erase(iter);
		}
	}
}
bool RoomSystem::LeaveRoomSystem(SessionInfo sessionInfo)
{
	SRWLockGuard<LOCK_TYPE::EXCLUSIVE> srwLockGuard(_sessionsLock);
	auto sessionToRoomIDIter = _sessions.find(sessionInfo.Id());
	if (sessionToRoomIDIter != _sessions.end())
	{
		int sessionRoomID = sessionToRoomIDIter->second;
		sessionToRoomIDIter->second = LEAVE_ROOM_SYSTEM;
		_pServer->ChangeRoomID(sessionInfo, LEAVE_ROOM_SYSTEM);
		if (sessionRoomID != CHANGING_ROOM_ID)
		{
			{
				SRWLockGuard<LOCK_TYPE::SHARED> srwLockGuard(_roomsLock);
				auto roomIter = _rooms.find(sessionRoomID);
				if (roomIter != _rooms.end())
				{
					roomIter->second->DoAsync(&Room::TryLeave, sessionInfo, LEAVE_ROOM_SYSTEM);
				}
			}
		}
		return true;
	}
	else
	{
		return false;
	}
}