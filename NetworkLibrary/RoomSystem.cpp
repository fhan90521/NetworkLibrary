#include "RoomSystem.h"
#include "IOCPServer.h"
#include "MyNew.h"
#include "Log.h"
int RoomSystem::RegisterRoom(const SharedPtr<Room>& pRoom)
{
	pRoom->_roomID = _newRoomID;
	pRoom->_pRoomSystem = this;
	{
		SRWLockGuard<LOCK_TYPE::EXCLUSIVE> srwLockGuard(_srwLock);
		_rooms[_newRoomID] = pRoom;
	}
	int ret = _newRoomID++;
	return ret;
}

void RoomSystem::DeregisterRoom(int roomID)
{
	SRWLockGuard<LOCK_TYPE::EXCLUSIVE> srwLockGuard(_srwLock);
	_rooms.erase(roomID);
}

void RoomSystem::UpdateRooms()
{
	while (bShutDown==false)
	{
		{
			SRWLockGuard<LOCK_TYPE::EXCLUSIVE> srwLockGuard(_srwLock);
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

RoomSystem::RoomSystem()
{
	InitializeSRWLock(&_srwLock);
	_roomUpdateThread = New<std::thread>(&RoomSystem::UpdateRooms, this);
}

RoomSystem::~RoomSystem()
{
	bShutDown = true;
	_roomUpdateThread->join();
	Delete<std::thread>(_roomUpdateThread);
}

int RoomSystem::GetSessionCntInRoomSystem()
{
	return _sessionToRoomID.size();
}

void RoomSystem::EnterRoom(SessionInfo sessionInfo, Room* beforeRoom, int afterRoomID)
{
	bool bError = false;

	{
		SRWLockGuard<LOCK_TYPE::EXCLUSIVE> srwLockGuard(_srwLock);
		auto sessionToRoomIDIter = _sessionToRoomID.find(sessionInfo.Id());
		if (sessionToRoomIDIter != _sessionToRoomID.end())
		{
			if (sessionToRoomIDIter->second == CHANGING_ROOM_ID)
			{
				auto afterRoomsIter = _rooms.find(afterRoomID);
				if (afterRoomsIter != _rooms.end())
				{
					sessionToRoomIDIter->second = afterRoomID;
					afterRoomsIter->second->DoAsync(&Room::TryEnter, sessionInfo);
					return;
				}
				else
				{
					beforeRoom->DoAsync(&Room::TryEnter, sessionInfo);
					return;
				}
			}
			else if (sessionToRoomIDIter->second == LEAVE_ROOM_SYSTEM)
			{
				OnLeaveByChangingRoomSession(sessionInfo);
				_sessionToRoomID.erase(sessionToRoomIDIter);
			}
			else
			{
				//로직상 불가능한 경우
				bError = true;
				Log::LogOnFile(Log::SYSTEM_LEVEL, "EnterRoom Impossible Error 1");
			}
		}
		else
		{
			//로직상 불가능한 경우
			bError = true;
			Log::LogOnFile(Log::SYSTEM_LEVEL, "EnterRoom Impossible Error 2");
		}
	}

	if (bError ==true)
	{
		OnError(sessionInfo,RoomError::ENTER_ROOM_ERROR);
	}
}

bool RoomSystem::ChangeRoom(SessionInfo sessionInfo,Room* beforeRoom, int afterRoomID)
{
	bool bError = false;
	bool ret = false;

	{
		SRWLockGuard<LOCK_TYPE::EXCLUSIVE> srwLockGuard(_srwLock);
		auto sessionToRoomIDIter = _sessionToRoomID.find(sessionInfo.Id());
		if (sessionToRoomIDIter != _sessionToRoomID.end())
		{
			if (sessionToRoomIDIter->second == beforeRoom->GetRoomID())
			{
				auto afterRoomIter = _rooms.find(afterRoomID);
				if (afterRoomIter != _rooms.end())
				{

					sessionToRoomIDIter->second = CHANGING_ROOM_ID;
					beforeRoom->DoAsync(&Room::Leave, sessionInfo, afterRoomID);
					ret = true;
				}
			}
			else
			{
				//로직상 불가능한 경우
				bError = true;
				Log::LogOnFile(Log::SYSTEM_LEVEL, "ChangeRoom Impossible Error");
			}
		}
	}

	if (bError == true)
	{
		OnError(sessionInfo,RoomError::CHANGE_ROOM_ERROR);
	}
	return ret;
}

bool RoomSystem::EnterRoomSystem(SessionInfo sessionInfo, int roomID)
{
	
	//어느 룸에도 속해있지 않고 처음 룸에 입장
	bool ret = false;
	SRWLockGuard<LOCK_TYPE::EXCLUSIVE> srwLockGuard(_srwLock);
	auto sessionToRoomIDIter = _sessionToRoomID.find(sessionInfo.Id());
	if (sessionToRoomIDIter == _sessionToRoomID.end())
	{
		auto roomsIter = _rooms.find(roomID);
		if (roomsIter != _rooms.end())
		{
			ret = true;
			_sessionToRoomID[sessionInfo.Id()] = roomID;
			roomsIter->second->DoAsync(&Room::TryEnter, sessionInfo);
		}
	}
	return ret;
}

void RoomSystem::LeaveRoomSystem(SessionInfo sessionInfo)
{
	SRWLockGuard<LOCK_TYPE::EXCLUSIVE> srwLockGuard(_srwLock);
	auto sessionToRoomIDIter = _sessionToRoomID.find(sessionInfo.Id());
	if (sessionToRoomIDIter != _sessionToRoomID.end())
	{
		int sessionRoomID = sessionToRoomIDIter->second;
		if (sessionRoomID != CHANGING_ROOM_ID)
		{
			auto roomIter = _rooms.find(sessionRoomID);
			if (roomIter != _rooms.end())
			{
				roomIter->second->DoAsync(&Room::LeaveRoomSystem, sessionInfo);
			}
			_sessionToRoomID.erase(sessionToRoomIDIter);
		}
		else
		{
			sessionToRoomIDIter->second = LEAVE_ROOM_SYSTEM;
		}
	}
}
