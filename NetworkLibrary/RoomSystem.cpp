#include "RoomSystem.h"

int RoomSystem::RegisterRoom(const SharedPtr<Room>& pRoom)
{
	pRoom->_roomID = _newRoomID;
	pRoom->_pRoomSystem = this;
	{
		EXCLUSIVE_LOCK_IDX(LOCK_ROOMS);
		_rooms[_newRoomID] = pRoom;
	}
	int ret = _newRoomID++;
	return ret;
}

void RoomSystem::DeregisterRoom(int roomID)
{
	EXCLUSIVE_LOCK_IDX(LOCK_ROOMS);
	_rooms.erase(roomID);
}

void RoomSystem::UpdateRooms()
{
	while (bShutDown==false)
	{
		{
			SHARED_LOCK_IDX(LOCK_ROOMS)
			LONG64 currentTime = GetTickCount64();
			for (auto&  temp: _rooms)
			{
				SharedPtr<Room>& pRoom = temp.second;
				if (pRoom->_bUpdating == false)
				{
					LONG64 timeDiff = currentTime - pRoom->_prevUpdateTime;
					if (timeDiff >= _updatePeriod)
					{
						pRoom->DoAsync(&Room::UpdateJob);
					}
				}
			}
		}
		Sleep(_updatePeriod / 4);
	}
}

RoomSystem::RoomSystem():_roomThread(&RoomSystem::UpdateRooms,this)
{
}

RoomSystem::~RoomSystem()
{
	bShutDown = true;
	_roomThread.join();
}

void RoomSystem::ChangeRoom(SessionInfo sessionInfo, int& beforeRoomID, int& afterRoomID)
{
	EXCLUSIVE_LOCK_IDX(LOCK_SESSION_TO_ROOM);
	auto sessionToRoomIDIter=_sessionToRoomID.find(sessionInfo.id);
	if (sessionToRoomIDIter != _sessionToRoomID.end())
	{
		if (sessionToRoomIDIter->second == beforeRoomID)
		{
			SHARED_LOCK_IDX(LOCK_ROOMS);
			auto beforeRoomIter = _rooms.find(beforeRoomID);
			auto afterRoomIter = _rooms.find(afterRoomID);
			if (beforeRoomIter != _rooms.end())
			{
				//동기로 떠나는 Room의 OnLeave를 호출 ChangeRoom은 Room안에서만 호출가능
				beforeRoomIter->second->Leave(sessionInfo);
			}
			else
			{
				beforeRoomID = INVALID_ROOM_ID;
			}

			if (afterRoomIter != _rooms.end())
			{
				//비동기 처리
				_sessionToRoomID[sessionInfo.id] = afterRoomID;
				afterRoomIter->second->DoAsync(&Room::TryEnter,sessionInfo);
			}
			else
			{
				afterRoomID = beforeRoomID;
			}
		}
		else
		{
			afterRoomID = beforeRoomID = sessionToRoomIDIter->second;
		}
	}
	else
	{
		beforeRoomID = beforeRoomID = INVALID_ROOM_ID;
	}

	if (beforeRoomID == afterRoomID&& beforeRoomID == INVALID_ROOM_ID && sessionToRoomIDIter != _sessionToRoomID.end())
	{
		_sessionToRoomID.erase(sessionToRoomIDIter);
	}
}

bool RoomSystem::EnterRoomSystem(SessionInfo sessionInfo, int roomID)
{
	
	//어느 룸에도 속해있지 않고 처음 룸에 입장
	bool ret = false;
	EXCLUSIVE_LOCK_IDX(LOCK_SESSION_TO_ROOM);
	auto sessionToRoomIDIter = _sessionToRoomID.find(sessionInfo.id);
	if (sessionToRoomIDIter == _sessionToRoomID.end())
	{
		SHARED_LOCK_IDX(LOCK_ROOMS);
		auto roomsIter = _rooms.find(roomID);
		if (roomsIter != _rooms.end())
		{
			ret = true;
			_sessionToRoomID[sessionInfo.id] = roomID;
			roomsIter->second->EnterRoom(sessionInfo);
		}
	}
	return ret;
}

void RoomSystem::LeaveRoomSystem(SessionInfo sessionInfo)
{
	EXCLUSIVE_LOCK_IDX(LOCK_SESSION_TO_ROOM);
	auto sessionToRoomIDIter = _sessionToRoomID.find(sessionInfo.id);
	if (sessionToRoomIDIter != _sessionToRoomID.end())
	{
		{
			SHARED_LOCK_IDX(LOCK_ROOMS);
			int sessionRoomID = sessionToRoomIDIter->second;
			auto roomIter = _rooms.find(sessionRoomID);
			if (roomIter != _rooms.end())
			{
				roomIter->second->TryDoSync(&Room::LeaveRoomSystem,sessionInfo);
			}
		}
		_sessionToRoomID.erase(sessionToRoomIDIter);
	}
}
