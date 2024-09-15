#pragma once
#include <memory>
#include "OS/MyWindow.h"
#include "Network/Session.h"
#include "Job/JobQueue.h"
#include "Container/MyStlContainer.h"

class Room: public JobQueue
{
private:
	virtual void Update(float deltaTime) = 0;
	virtual void OnEnter(SessionInfo sessionInfo) = 0;
	virtual bool CheckCanLeave(SessionInfo sessionInfo) = 0;
	virtual void OnLeave(SessionInfo sessionInfo) = 0;
protected:
	//룸 내부에서만 호출되어야 한다
	bool ChangeRoom(SessionInfo sessionInfo, int afterRoomID);
public:
	typedef int ID;
	Room(HANDLE hCompletionPort);
	virtual ~Room();
private:
	friend class RoomSystem;
	class RoomSystem* _pRoomSystem=nullptr;
	LONG _updateCnt = 0;
	int _sessionCnt = 0;
	HashSet<SessionInfo::ID>_sessionsInRoom;
	CHAR _bUpdating = false;
	int _roomID;
	ULONG64 _prevUpdateTime = 0;
	HashMap<SessionInfo::ID,int>_tryLeaveSessions;
	void ProcessLeave();
	void Enter(SessionInfo sessionInfo);
	void TryLeave(SessionInfo sessionInfo, int afterRoomID);
	void LeaveAndEnter(SessionInfo sessionInfo,int afterRoomID);
	void UpdateJob();
public:
	int GetUpdateCnt();
	int GetSessionCnt();
	int GetRoomID();
};