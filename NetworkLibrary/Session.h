#pragma once
#include "MyWindow.h"
#include "CRingBuffer.h"
#include "CSendBuffer.h"
#include "LockQueue.h"
#include "MPSCQueue.h"
#define MAX_SEND_BUF_CNT 512

union SessionInfo
{
public:
	typedef unsigned long long ID;
private:
	struct Index
	{
	private:
		USHORT reserved1;
		USHORT reserved2;
		USHORT reserved3;
	public:
		USHORT val;
	};
private:
	friend class IOCPServer;
	friend class IOCPDummyClient;
	Index index;
	unsigned long long id;
public:
	unsigned long long Id()
	{
		return id;
	}
	friend bool operator == (const SessionInfo& left, const SessionInfo& right)
	{
		return left.id == right.id;
	}
	SessionInfo(const SessionInfo::ID otherId)
	{
		id = otherId;
	}
	SessionInfo() = default;
};

struct SessionManageInfo
{
	SHORT bDeallocated = true;
	SHORT refCnt = 0;
};

struct Session
{
	SOCKET socket;
	SessionInfo sessionInfo;
	SessionManageInfo sessionManageInfo;

	CHAR bReservedDisconnect = false;
	CHAR onConnecting;
	CHAR bSending;
	SHORT sendBufCnt = 0;
	OVERLAPPED sendOverLapped;
	MPSCQueue<CSendBuffer*> sendBufQ;
	CSendBuffer** pSendedBufArr;

	OVERLAPPED recvOverLapped;
	CRingBuffer recvBuffer;

	char ip[INET_ADDRSTRLEN] = "\0";
	USHORT port = 0;

	Session()
	{
		pSendedBufArr = new CSendBuffer*[MAX_SEND_BUF_CNT];
	}
	~Session()
	{
		delete pSendedBufArr;
	}
};
