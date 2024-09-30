#pragma once
#include "Room.h"
#include "Network/Session.h"
#include "Container/MyStlContainer.h"
#include "Container/MPSCQueue.h"
#include "Lock/LockGuard.h"
#include <thread>
#include <atomic>
//등록되어있는 Room보다 수명이 긴걸 보장해야함
class RoomSystem: public JobQueue
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
	class IOCPServer* _pServer = nullptr;
	std::thread* _roomUpdateThread = nullptr;
	bool bShutDown = false;
	int _updatePeriod=15;
	Stack<int> _validRoomIDs;
	HashMap<Room::ID, SharedPtr<Room>> _rooms;
	HashMap<SessionInfo::ID, Room::ID> _sessions;
	SRWLOCK _roomsLock;
	SRWLOCK _sessionsLock;
	MPSCQueue<SessionInfo::ID> registerQueue;
	List <SessionInfo::ID> _tryLeaveSessions;
	size_t _tryLeaveSessionsCnt = 0;
	void UpdateRooms();
	void EnterRoom(SessionInfo sessionInfo,Room* beforeRoom ,int afterRoomID);
	bool ChangeRoom(SessionInfo sessionInfo, Room* beforeRoom, int afterRoomID);
	void RegisterLeaveSession(SessionInfo sessionInfo);
	void ProcessLeaveSystem();
public:
	void SetUpdatePeriod(int updatePeriod)
	{
		_updatePeriod = updatePeriod;
	}
	void Run();
	bool LeaveRoomSystem(SessionInfo sessionInfo);
	bool EnterRoomSystem(SessionInfo sessionInfo, int roomID);
	RoomSystem(IOCPServer* pServer, HANDLE hCompletionPort);
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
	virtual void OnRegisterToLeave(SessionInfo sessionInfo) = 0;
	virtual void OnLeaveRoomSystem(SessionInfo sessionInfo)=0;
	virtual bool CheckCanLeaveSystem(SessionInfo sessionInfo) = 0;
};