#pragma once
#include "Room.h"
#include "MyStlContainer.h"
#include "LockGuard.h"
#include <thread>
#include <atomic>
#include "Session.h"
//등록되어있는 Room보다 수명이 긴걸 보장해야함
class RoomSystem
{
public:
	enum : int
	{
		INVALID_ROOM_ID = -1,
		CHANGING_ROOM_ID = -2,
		LEAVE_ROOM_SYSTEM = -3,
		MAX_ROOM_ID= 10000,
	};
private:
	friend class Room;
	class IOCPServer* _pServer;
	std::thread* _roomUpdateThread;
	bool bShutDown = false;
	int _updatePeriod=15;
	alignas(64) Stack<int> _validRoomIDs;
	HashMap<Room::ID, SharedPtr<Room>> _rooms;
	alignas(64) HashMap<SessionInfo::ID, Room::ID> _sessions;
	SRWLOCK _roomsLock;
	SRWLOCK _sessionsLock;
	void UpdateRooms();
	void EnterRoom(SessionInfo sessionInfo,Room* beforeRoom ,int afterRoomID);
	bool ChangeRoom(SessionInfo sessionInfo, Room* beforeRoom, int afterRoomID);
public:
	void SetUpdatePeriod(int updatePeriod)
	{
		_updatePeriod = updatePeriod;
	}
	bool LeaveRoomSystem(SessionInfo sessionInfo);
	bool EnterRoomSystem(SessionInfo sessionInfo, int roomID);
	RoomSystem(IOCPServer* pServer);
	virtual ~RoomSystem();
	int GetSessionCntInRoomSystem();
	bool RegisterRoom(const SharedPtr<Room>& pRoom);
	void DeregisterRoom(const SharedPtr<Room>& pRoom);
	void CloseRoomSystem();
public:
	enum class RoomError
	{
		ENTER_ROOM_ERROR,
		CHANGE_ROOM_ERROR,
	};
private:
	virtual void OnLeaveByChangingRoomSession(SessionInfo sessionInfo)=0;
	virtual void OnError(SessionInfo sessionInfo, RoomError error) = 0;
};