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
	USE_LOCK;
	friend class Room;
	class IOCPServer* _pServer = nullptr;
	std::jthread _roomThread;
	HashMap<Room::ID,SharedPtr<Room>> _rooms;
	HashMap<SessionInfo::ID, Room::ID> _sessionToRoomID;
	bool bShutDown = false;
	int _updatePeriod=15;
	int _newRoomID = 0;
	void UpdateRooms();
	void EnterRoom(SessionInfo sessionInfo,Room* beforeRoom ,int afterRoomID);
	bool ChangeRoom(SessionInfo sessionInfo, Room* beforeRoom, int afterRoomID);
public:
	void SetUpdatePeriod(int updatePeriod)
	{
		_updatePeriod = updatePeriod;
	}
	void LeaveRoomSystem(SessionInfo sessionInfo);
	bool EnterRoomSystem(SessionInfo sessionInfo, int roomID);
	RoomSystem(class IOCPServer* pServer);
	virtual ~RoomSystem();

	int RegisterRoom(const SharedPtr<Room>& pRoom);
	void DeregisterRoom(int roomID);
};