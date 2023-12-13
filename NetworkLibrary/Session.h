#pragma once
#include "MyWindow.h"
#include "CRingBuffer.h"
union SessionInfo
{
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
	SessionInfo sessionInfo;
	SOCKET socket;

	OVERLAPPED sendOverLapped;
	CRingBuffer sendBuffer;
	SHORT sendBufCnt=0;

	OVERLAPPED recvOverLapped;
	CRingBuffer recvBuffer;

	SRWLOCK sessionLock;
	LONG bSending;

	SessionManageInfo sessionManageInfo;

	bool bConnecting;
	char ip[INET_ADDRSTRLEN] = "\0";
	USHORT port=0;
	Session()
	{
		InitializeSRWLock(&sessionLock);
	}
};
