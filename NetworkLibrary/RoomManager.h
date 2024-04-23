#pragma once
#include "Room.h"
#include "MyStlContainer.h"
#include "LockGuard.h"
#include <thread>
class RoomManager
{
private:
	USE_MUTEX
	friend class Room;
	std::jthread _roomThread;
	List<Room*> _pRoomList;
	bool bShutDown = false;
	int _updatePeriod=30;
	void RegisterRoom(Room* pRoom);
	void DeregisterRoom(Room* pRoom);
	void UpdateRooms();
	RoomManager();
	~RoomManager();
public:
	static RoomManager* GetInstance()
	{
		static RoomManager _instance;
		return &_instance;
	}
	void SetUpdatePeriod(int updatePeriod)
	{
		_updatePeriod = updatePeriod;
	}
};