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
	// 아직 Room의 동적 생성및 파괴에대한 필요성을 느끼지 못하여 해당기능에 대한건 완벽히 구현하지 않았음<- 방을 풀링하여 사용하면 되지 않을까 생각함 update쪽에서 빈방처리를 하면 될거라 생각함
	// 방을 옮기다 옮기기전의 방과 옮기고 싶은방이 
	// 둘다 사라져 있을때 해당 세션을 disconnect시키는데 이때 LeaveRoomSytem을 호출하면서 재귀락이 걸릴 수 있다<-훗날 방의 동적 생성및 파괴를 구현할 일이 생김을 대비하여 미리 recursive_mutex사용  
	USE_RECURSIVE_MUTEX;
	friend class Room;
	class IOCPServer* _pServer = nullptr;
	std::jthread _roomThread;
	HashMap<Room::ID,SharedPtr<Room>> _rooms;
	HashMap<SessionInfo::ID, Room::ID> _sessionToRoomID;
	bool bShutDown = false;
	int _updatePeriod=30;
	int _newRoomID = 0;
	void UpdateRooms();
	void EnterRoom(SessionInfo sessionInfo,int beforeRoomID ,int afterRoomID);
	void ChangeRoom(SessionInfo sessionInfo, int& beforeRoomID, int& afterRoomID);
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