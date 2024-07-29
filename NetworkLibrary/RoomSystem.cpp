#include "RoomSystem.h"

int RoomSystem::RegisterRoom(const SharedPtr<Room>& pRoom)
{
	pRoom->_roomID = _newRoomID;
	pRoom->_pRoomSystem = this;
	{
		RECURSIVE_LOCK;
		_rooms[_newRoomID] = pRoom;
	}
	int ret = _newRoomID++;
	return ret;
}

void RoomSystem::DeregisterRoom(int roomID)
{
	RECURSIVE_LOCK;
	_rooms.erase(roomID);
}

void RoomSystem::UpdateRooms()
{
	while (bShutDown==false)
	{
		{
			RECURSIVE_LOCK
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
		Sleep(_updatePeriod / 4);
	}
}

RoomSystem::RoomSystem(IOCPServer* pServer):_roomThread(&RoomSystem::UpdateRooms,this)
{
	_pServer = pServer;
}

RoomSystem::~RoomSystem()
{
	bShutDown = true;
	_roomThread.join();
}

void RoomSystem::EnterRoom(SessionInfo sessionInfo, int beforeRoomID, int afterRoomID)
{
	RECURSIVE_LOCK;
	auto sessionToRoomIDIter = _sessionToRoomID.find(sessionInfo.id);
	if (sessionToRoomIDIter != _sessionToRoomID.end())
	{
		if (sessionToRoomIDIter->second == CHANGING_ROOM_ID)
		{
			auto afterRoomsIter = _rooms.find(afterRoomID);
			if (afterRoomsIter != _rooms.end())
			{
				_sessionToRoomID[sessionInfo.id] = afterRoomID;
				afterRoomsIter->second->DoAsync(&Room::TryEnter, sessionInfo);
				return;
			}
			else
			{
				auto beforeRoomIter = _rooms.find(beforeRoomID);
				if (beforeRoomIter != _rooms.end())
				{
					beforeRoomIter->second->DoAsync(&Room::TryEnter, sessionInfo);
					return;
				}
			}
		}
		else
		{
			_pServer->Disconnect(sessionInfo);
		}
	}
}

void RoomSystem::ChangeRoom(SessionInfo sessionInfo, int& beforeRoomID, int& afterRoomID)
{
	RECURSIVE_LOCK;
	auto sessionToRoomIDIter=_sessionToRoomID.find(sessionInfo.id);
	if (sessionToRoomIDIter != _sessionToRoomID.end())
	{
		if (sessionToRoomIDIter->second == beforeRoomID)
		{
			auto beforeRoomIter = _rooms.find(beforeRoomID);
			auto afterRoomIter = _rooms.find(afterRoomID);
			if (beforeRoomIter != _rooms.end()&& afterRoomIter != _rooms.end())
			{	

				_sessionToRoomID[sessionInfo.id] = CHANGING_ROOM_ID;
				int ret =_sessionToRoomID[sessionInfo.id];
				beforeRoomIter->second->DoAsync(&Room::Leave,sessionInfo, afterRoomID);
				afterRoomID = CHANGING_ROOM_ID;
			}
			else
			{
				//전이 없을때
				if (beforeRoomIter == _rooms.end())
				{
					beforeRoomID = INVALID_ROOM_ID;
				}
				//후가 없을때
				if (afterRoomIter == _rooms.end())
				{
					afterRoomID = INVALID_ROOM_ID;
				}

				if (afterRoomID == INVALID_ROOM_ID && beforeRoomID == INVALID_ROOM_ID)
				{
					_sessionToRoomID.erase(sessionToRoomIDIter);
				}
			}
		}
		else
		{
			afterRoomID = beforeRoomID = sessionToRoomIDIter->second;
		}
	}
	else
	{
		beforeRoomID = afterRoomID = INVALID_ROOM_ID;
	}
}

bool RoomSystem::EnterRoomSystem(SessionInfo sessionInfo, int roomID)
{
	
	//어느 룸에도 속해있지 않고 처음 룸에 입장
	bool ret = false;
	RECURSIVE_LOCK;
	auto sessionToRoomIDIter = _sessionToRoomID.find(sessionInfo.id);
	if (sessionToRoomIDIter == _sessionToRoomID.end())
	{
		auto roomsIter = _rooms.find(roomID);
		if (roomsIter != _rooms.end())
		{
			ret = true;
			_sessionToRoomID[sessionInfo.id] = roomID;
			roomsIter->second->DoAsync(&Room::TryEnter, sessionInfo);
		}
	}
	return ret;
}

void RoomSystem::LeaveRoomSystem(SessionInfo sessionInfo)
{
	RECURSIVE_LOCK;
	auto sessionToRoomIDIter = _sessionToRoomID.find(sessionInfo.id);
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
		}
		_sessionToRoomID.erase(sessionToRoomIDIter);
	}
}
