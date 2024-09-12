#pragma once
#include <memory>
#include "MyWindow.h"
#include "Session.h"
#include "JobQueue.h"
#include "MyStlContainer.h"

class Room: public JobQueue
{
public:
	typedef int ID;
	Room(HANDLE hCompletionPort);
	virtual ~Room();
private:
	virtual void Update(float deltaTime) = 0;
	virtual void OnEnter(SessionInfo sessionInfo) = 0;
	virtual int RequestEnter(SessionInfo sessionInfo) = 0;
	virtual void OnLeave(SessionInfo sessionInfo) = 0;
	virtual void OnLeaveRoomSystem(SessionInfo sessionInfo, bool bEnterCompleted) = 0;
protected:
	//룸 내부에서만 호출되어야 한다
	bool ChangeRoom(SessionInfo sessionInfo, int afterRoomID);
private:
	friend class RoomSystem;
	class RoomSystem* _pRoomSystem=nullptr;
	LONG _updateCnt = 0;
	int _sessionCnt = 0;
	HashSet<SessionInfo::ID>_tryEnterSessions;
	HashSet<SessionInfo::ID>_sessionsInRoom;
	CHAR _bUpdating = false;
	int _roomID;
	ULONG64 _prevUpdateTime = 0;
	//List<SessionInfo>_tryLeaveSessions;
	void ProcessEnter();
	//void ProcessLeave();
	void TryEnter(SessionInfo sessionInfo);
	void Leave(SessionInfo sessionInfo,int afterRoomID);
	void LeaveRoomSystem(SessionInfo sessionInfo);
	void UpdateJob();
	
	//virtual bool RequestLeave(SessionInfo sessionInfo) = 0;
public:
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