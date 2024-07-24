#pragma once
#include "Room.h"
#include "MyStlContainer.h"
#include "LockGuard.h"
#include <thread>
#include "Session.h"
//등록되어있는 Room보다 수명이 긴걸 보장해야함
class RoomSystem
{
private:
	enum 
	{
		LOCK_ROOMS,
		LOCK_SESSION_TO_ROOM
	};
	USE_RECURSIVE_MUTEX(2);
	friend class Room;
	std::jthread _roomThread;
	HashMap<Room::ID,SharedPtr<Room>> _rooms;
	HashMap<SessionInfo::ID, Room::ID> _sessionToRoomID;
	bool bShutDown = false;
	int _updatePeriod=30;
	int _newRoomID = 0;
	void UpdateRooms();
	void ChangeRoom(SessionInfo sessionInfo, int& beforeRoomID, int& afterRoomID);
public:
	void SetUpdatePeriod(int updatePeriod)
	{
		_updatePeriod = updatePeriod;
	}
	void LeaveRoomSystem(SessionInfo sessionInfo);
	bool EnterRoomSystem(SessionInfo sessionInfo, int roomID);
	RoomSystem();
	virtual ~RoomSystem();

	int RegisterRoom(const SharedPtr<Room>& pRoom);
	void DeregisterRoom(int roomID);
};