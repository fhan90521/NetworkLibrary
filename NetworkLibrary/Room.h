#pragma once
#include "RoomJob.h"
#include "LockFreeQueue.h"
#include <memory>
#include "MyWindow.h"
#include "IOCPServer.h"
class Room
{
private:
	friend class IOCPServer;
	char _bProcessing = false;
	ULONG64 _prevUpdateTime = 0;
	ULONG64 _updatePeriod = 0;
	LockFreeQueue<RoomJob*> _jobQueue;
	List<SessionInfo>_tryEnterSessions;
	List<SessionInfo>_tryLeaveSessions;
	ULONG64 _currentTime = 0;
	void ProcessJob();
	void	ProcessRoom();
	void ProcessEnter();
	void ProcessLeave();
	void TryEnter(SessionInfo sessionInfo);
	void TryLeave(SessionInfo sessionInfo);
protected:
	virtual void Update() = 0;
	virtual void OnEnter(SessionInfo sessionInfo) = 0;
	virtual int RequestEnter(SessionInfo sessionInfo) = 0;
	virtual void OnLeave(SessionInfo sessionInfo) = 0;
	virtual bool RequestLeave(SessionInfo sessionInfo) = 0;
public:
	virtual ~Room();
	Room() {};
	void MakeRoomJob(CallbackType&& callback);
	template<typename T, typename Ret, typename... Args>
	void MakeRoomJob(Ret(T::* memFunc)(Args...), Args... args);
	void EnterRoom(SessionInfo sessionInfo);
	void LeaveRoom(SessionInfo sessionInfo);
	enum
	{
		ENTER_SUCCESS,
		ENTER_DENIED,
		ENTER_HOLD
	};
};