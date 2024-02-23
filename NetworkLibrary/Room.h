#pragma once
#include "RoomJob.h"
#include "MpscQueue.h"
#include <memory>
#include "MyWindow.h"
#include "IOCPServer.h"
class Room
{
private:
	friend class IOCPServer;
	MpscQueue<RoomJob*> _jobQueue;
	alignas(64) char _bProcessing = false;
	alignas(64) char  _isUpdateTime = false;
	IOCPServer* _pServer=nullptr;
	ULONG64 _lastProcessTime=0;
	void ProcessJob();
	void	ProcessRoom();
protected:
	virtual void Update(float deltaTime) = 0;
	void operator delete (void* p)
	{
		free(p);
	}
public:
	virtual ~Room();
	Room(){};
	void MakeRoomJob(CallbackType&& callback);
	template<typename T, typename Ret, typename... Args>
	void MakeRoomJob(Ret(T::* memFunc)(Args...), Args... args);
};