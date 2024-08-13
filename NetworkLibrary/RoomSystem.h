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
	enum: int
	{
		INVALID_ROOM_ID = -1,
		CHANGING_ROOM_ID = -2,
		LEAVE_ROOM_SYSTEM = -3
	};
	SRWLOCK _srwLock;
	friend class Room;
	class IOCPServer* _pServer = nullptr;
	std::thread* _roomUpdateThread;
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
private:
	virtual void OnLeaveByChangingRoomSession(SessionInfo sessionInfo)=0;
};