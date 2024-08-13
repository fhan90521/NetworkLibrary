#pragma once
#include <memory>
#include "MyWindow.h"
#include "Session.h"
#include "JobQueue.h"
#include "MyStlContainer.h"

class Room: public JobQueue
{
private:
	enum : int
	{
		INVALID_ROOM_ID = -1,
		CHANGING_ROOM_ID = -2,
		LEAVE_ROOM_SYSTEM = -3
	};
	friend class RoomSystem;
	class RoomSystem* _pRoomSystem=nullptr;
	LONG _updateCnt = 0;
	int _sessionCnt = 0;
	HashSet<SessionInfo::ID>_tryEnterSessions;
	HashSet<SessionInfo::ID>_sessionsInRoom;
	CHAR _bUpdating = false;
	int _roomID= INVALID_ROOM_ID;
	ULONG64 _prevUpdateTime = 0;
	//List<SessionInfo>_tryLeaveSessions;
	void ProcessEnter();
	//void ProcessLeave();
	void TryEnter(SessionInfo sessionInfo);
	void Leave(SessionInfo sessionInfo,int afterRoomID);
	void LeaveRoomSystem(SessionInfo sessionInfo);
	void UpdateJob();

private:
	virtual void Update(float deltaTime) = 0;
	virtual void OnEnter(SessionInfo sessionInfo) = 0;
	virtual int RequestEnter(SessionInfo sessionInfo) = 0;
	virtual void OnLeave(SessionInfo sessionInfo) = 0;
	virtual void OnLeaveRoomSystem(SessionInfo sessionInfo) = 0;

protected:
	//룸 내부에서만 호출되어야 한다
	bool ChangeRoom(SessionInfo sessionInfo, int afterRoomID);
	
	//virtual bool RequestLeave(SessionInfo sessionInfo) = 0;
public:
	typedef int ID;
	virtual ~Room();
	Room(HANDLE hCompletionPort);
	enum: int
	{
		ENTER_DENIED,
		ENTER_SUCCESS,
		ENTER_HOLD
	};
	int GetUpdateCnt();
	int GetSessionCnt();
	int GetRoomID();
};