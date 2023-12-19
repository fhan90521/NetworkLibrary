#pragma once
#include "MyWindow.h"
#include "CRingBuffer.h"
#include "CLockQueue.h"
#include "CSerialBuffer.h"
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
	CLockQueue<CSerialBuffer*> sendBufQ;
	SHORT sendBufCnt=0;

	OVERLAPPED recvOverLapped;
	CRingBuffer recvBuffer;

	LONG bSending;

	SessionManageInfo sessionManageInfo;

	bool bConnecting;
	char ip[INET_ADDRSTRLEN] = "\0";
	USHORT port=0;
};
