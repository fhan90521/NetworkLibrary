#pragma once
#include "MyWindow.h"
#include "CRingBuffer.h"
#include "CSendBuffer.h"
#include "LockFreeQueue.h"
#include "CLockArrayQueue.h"
#include "LockFreeQueueBasic.h"
#define MAX_SEND_BUF_CNT 512
union SessionInfo
{
	typedef unsigned long long ID;
	struct Index
	{
	private:
		unsigned short reserved1;
		unsigned short reserved2;
		unsigned short reserved3;
	public:
		unsigned short val;
	};
	Index index;
	unsigned long long id;
};
struct SessionManageInfo
{
	SHORT bDeallocated = true;
	SHORT refCnt = 0;
};
struct Session
{
	SessionManageInfo sessionManageInfo;
	SessionInfo sessionInfo;
	SOCKET socket;

	LONG bSending;
	SHORT sendBufCnt = 0;
	OVERLAPPED sendOverLapped;
	CLockArrayQueue<CSendBuffer*> sendBufQ;
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
