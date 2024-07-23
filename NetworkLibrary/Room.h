#pragma once
#include <memory>
#include "MyWindow.h"
#include "Session.h"
#include "JobQueue.h"
#include "MyStlContainer.h"
#define INVALID_ROOM_ID -1
class Room: public JobQueue
{
private:
	friend class RoomSystem;
	class RoomSystem* _pRoomSystem;
	LONG _updateCnt = 0;
	int _sessionCnt = 0;
	HashSet<SessionInfo::ID>_tryEnterSessions;
	HashSet<SessionInfo::ID>_sessionsInRoom;
	CHAR _bUpdating = false;
	unsigned short _roomID;
	//List<SessionInfo>_tryLeaveSessions;
	void ProcessEnter();
	//void ProcessLeave();
	void TryEnter(SessionInfo sessionInfo);
	void Leave(SessionInfo sessionInfo);
	void LeaveRoomSystem(SessionInfo sessionInfo);
	void EnterRoom(SessionInfo sessionInfo);
	void LeaveRoom(SessionInfo sessionInfo);
protected:
	ULONG64 _prevUpdateTime = 0;
	void UpdateJob();
	virtual void Update(float deltaTime) = 0;
	virtual void OnEnter(SessionInfo sessionInfo) = 0;
	virtual int RequestEnter(SessionInfo sessionInfo) = 0;
	virtual void OnLeave(SessionInfo sessionInfo) = 0;
	virtual void OnLeaveRoomSystem(SessionInfo sessionInfo) = 0;
	void ChangeRoom(SessionInfo sessionInfo, int& beforeRoomID, int& afterRoomID);
	//virtual bool RequestLeave(SessionInfo sessionInfo) = 0;
public:
	typedef unsigned short ID;
	virtual ~Room();
	Room(IOCPServer* pServer);
	enum
	{
		ENTER_DENIED,
		ENTER_SUCCESS,
		ENTER_HOLD
	};
	int GetUpdateCnt();
	int GetSessionCnt();
	int GetRoomID();
};