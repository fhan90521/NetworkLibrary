#pragma once
#include "CJob.h"
#include "MpscQueue.h"
#include <memory>
#include "MyWindow.h"
#include "IOCPServer.h"
class Room
{
private:
	friend class IOCPServer;
	MpscQueue<CJob*> _jobQueue;
	DWORD _clock = 0;
	bool _bReleased = false;
	char  _bProcessing = false;
	void ProcessJob();
protected:
	virtual ~Room();
	Room() {};
	void operator delete (void* p)
	{
		free(p);
	}
public:
	void DoAsync(CallbackType&& callback);

	template<typename T, typename Ret, typename... Args>
	void DoAsync(Ret(T::* memFunc)(Args...), Args... args);
};