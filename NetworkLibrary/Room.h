#pragma once
#include <memory>
#include "MyWindow.h"
#include "Session.h"
#include "JobQueue.h"
class Room: public JobQueue
{
private:
	friend class RoomManager;
	LONG _updateCnt = 0;
	HashSet<SessionInfo::ID>_tryEnterSessions;
	//List<SessionInfo>_tryLeaveSessions;
	void ProcessEnter();
	//void ProcessLeave();
	void TryEnter(SessionInfo sessionInfo);
	void Leave(SessionInfo sessionInfo);
protected:
	ULONG64 _currentTime = 0;
	ULONG64 _prevUpdateTime = 0;
	CHAR _bUpdating = false;
	void UpdateJob();
	virtual void Update() = 0;
	virtual void OnEnter(SessionInfo sessionInfo) = 0;
	virtual int RequestEnter(SessionInfo sessionInfo) = 0;
	virtual void OnLeave(SessionInfo sessionInfo) = 0;
	//virtual bool RequestLeave(SessionInfo sessionInfo) = 0;
public:
	virtual ~Room();
	Room(IOCPServer* pServer);
	void EnterRoom(SessionInfo sessionInfo);
	void LeaveRoom(SessionInfo sessionInfo);
	enum
	{
		ENTER_SUCCESS,
		ENTER_DENIED,
		ENTER_HOLD
	};
	int GetUpdateCnt();
};