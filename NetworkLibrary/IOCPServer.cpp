#define _CRT_SECURE_NO_WARNINGS
#include "IOCPServer.h"
#include <list>
#include <unordered_map>
#include <process.h>
#include <iostream>
#include <fstream>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include<conio.h>
#include<memory.h>
#include "PKT_TYPE.h"
//#include "Log.h"
using namespace rapidjson;
using namespace std;
#define DDD 00
#define MAX_SEND_BUF_CNT 500
#define EXIT_TIMEOUT 5000
#define SERVER_DOWN_KEY 200

HANDLE CreateNewCompletionPort(DWORD dwNumberOfConcurrentThreads);
BOOL AssociateDeviceWithCompletionPort(HANDLE hCompletionPort, HANDLE hDevice, ULONG_PTR dwCompletionKey);
HANDLE CreateNewCompletionPort(DWORD dwNumberOfConcurrentThreads)
{
	return(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL,
		0, dwNumberOfConcurrentThreads));
}
BOOL AssociateDeviceWithCompletionPort(HANDLE hCompletionPort, HANDLE hDevice, ULONG_PTR dwCompletionKey)
{
	HANDLE hcp = CreateIoCompletionPort(hDevice, hCompletionPort, dwCompletionKey, 0);
	return (hcp == hCompletionPort);
}
void ShowCurrentTime()
{
	__time64_t now = time(0);
	tm* timeinfo = _localtime64(&now);
	cout << "현재 시간: ";
	cout << (timeinfo->tm_year + 1900) << '-'
		<< (timeinfo->tm_mon + 1) << '-'
		<< timeinfo->tm_mday << ' '
		<< timeinfo->tm_hour << ':'
		<< timeinfo->tm_min << ':'
		<< timeinfo->tm_sec << '\n';
}

void IOCPServer::GetSeverSetValues()
{
	ifstream fin("ServerSetting.txt");
	if (!fin)
	{
		__debugbreak();
	}
	string json((istreambuf_iterator<char>(fin)), (istreambuf_iterator<char>()));
	fin.close();
	//cout << json << endl;

	Document d;
	d.Parse(json.data());

	Value& v = d["IOCP_THREAD_NUM"];
	IOCP_THREAD_NUM = v.GetInt();

	v = d["CONCURRENT_THREAD_NUM"];
	CONCURRENT_THREAD_NUM = v.GetInt();

	v = d["SERVER_PORT"];
	SERVER_PORT = v.GetInt();

	return;
}

Session* IOCPServer::FindSession(SessionInfo sessionInfo)
{
	Session* pSession = &_sessionArray[sessionInfo.index.val];
	InterlockedIncrement16(&pSession->sessionManageInfo.refCnt);
	if (pSession->sessionManageInfo.bDeallocated == true)
	{
		//Release가 이미 되고 있다. 이미되고 할당 받는중인데 Release가 되는 문제 <-문제 a -> 받는중이라도 release 될 일 없다.
		if (InterlockedDecrement16(&pSession->sessionManageInfo.refCnt) == 0)
		{
			ReleaseSession(pSession);
		}
		return nullptr;
	}

	//Release가 안되고 있다->bDeallocated==false case 1. 그냥 사용 정상인경우 , case 2 Release가 되고 또 이미 다른id로 사용된다 <-문제 b.
	if (pSession->sessionInfo.id != sessionInfo.id)
	{
		if (InterlockedDecrement16(&pSession->sessionManageInfo.refCnt) == 0)
		{
			ReleaseSession(pSession);
		}
		return nullptr;
	}
	return pSession;
}

Session* IOCPServer::AllocSession(SOCKET clientSock)
{
	AcquireSRWLockExclusive(&_stackLock);
	unsigned short index = _validIndexStack.top();
	_validIndexStack.pop();
	ReleaseSRWLockExclusive(&_stackLock);

	Session* pSession = &_sessionArray[index];
	//여기서 문제 a 발생 하는가 bDeallocated == true인상태->상관없다

	InterlockedIncrement16(&pSession->sessionManageInfo.refCnt);

	//문제 b에 의해 먼저 id를 할당하고 bDeallocated 변수를 바꾼다.

	pSession->sessionInfo.id = _newSessionID++;
	pSession->sessionInfo.index.val = index;
	//여기까지 bDeallocated == true이다
	pSession->sessionManageInfo.bDeallocated = false;
	return pSession;
}

void IOCPServer::ReleaseSession(Session* pSession)
{
	/*struct SessionManageInfo
	{
		SHORT refCnt = 0;
		SHORT bDeallocated = true;
	};*/
	if (InterlockedCompareExchange((LONG*)&pSession->sessionManageInfo, true, 0) != 0)
	{
		// refCnt ==0 && bDeallocated == false 이 아닌 경우
		return ;
	}
	SessionInfo sessionInfo = pSession->sessionInfo;
	closesocket(pSession->socket);
	CSerialBuffer* pBufs[MAX_SEND_BUF_CNT];
	int remainSize = pSession->sendBuffer.GetUseSize();
	int remainBufCnt = remainSize / sizeof(NetHeader*);
	pSession->sendBuffer.Dequeue((char*)pBufs, remainSize);
	for (int i = 0; i < remainBufCnt; i++)
	{
		pBufs[i]->DecrementRefCnt();
	}
	AcquireSRWLockExclusive(&_stackLock);
	_validIndexStack.push(pSession->sessionInfo.index.val);
	ReleaseSRWLockExclusive(&_stackLock);
	OnDisConnect(sessionInfo);
}

void IOCPServer::AcceptWork()
{
	SOCKET clientSock;
	struct sockaddr_in clientaddr;
	int addrlen = sizeof(clientaddr);
	char ip[INET_ADDRSTRLEN];
	USHORT port;

	while (1)
	{
		clientSock = accept(_listenSock, (struct sockaddr*)&clientaddr, &addrlen);
		//Log용
		_acceptCnt++;
		
		if (clientSock == INVALID_SOCKET)
		{
			int error = WSAGetLastError();
			if (error != WSAEINTR)
			{
				cout << "accept error: " << error << endl;
			}
			break;
		}
		if (_validIndexStack.empty())
		{
			closesocket(clientSock);
			continue;
		}


		inet_ntop(AF_INET, &clientaddr.sin_addr, ip, sizeof(ip));
		port = ntohs(clientaddr.sin_port);
		if (OnConnectRequest(ip, port) == false)
		{
			continue;
		}

		Session* pSession = AllocSession(clientSock);
		

		bool ret = AssociateDeviceWithCompletionPort(_hcp, (HANDLE)clientSock, (ULONG_PTR)pSession);
		if (ret == false)
		{
			cout << "AssociateDeviceWithCompletionPort error : " << WSAGetLastError() << endl;
			DebugBreak();
		}

		pSession->socket = clientSock;
		pSession->sendBuffer.ClearBuffer();
		pSession->recvBuffer.ClearBuffer();
		pSession->bConnecting = true;
		pSession->bSending = false;
		strcpy_s(pSession->ip, INET_ADDRSTRLEN, ip);
		pSession->port = port;

		//_LOG(dfLOG_LEVEL_DEBUG, "\n[TCP 서버] 클라이언트 접속; IP 주소=%s, 포트 번호=%d\n", pSession->ip, pSession->port);
		OnConnect(pSession->sessionInfo);

		RecvPost(pSession);
	}
	return;
}

void IOCPServer::CloseServer()
{
	for (int i = 0; i < _arraySize; i++)
	{
		Session* pSession = &_sessionArray[i];
		closesocket(pSession->socket);
	}
	closesocket(_listenSock);
	for (int i = 0; i < IOCP_THREAD_NUM; i++)
	{
		PostQueuedCompletionStatus(_hcp, 0, SERVER_DOWN_KEY, 0);
	}
	int i = 0;
	for (HANDLE hThread : _hThreadList)
	{
		DWORD retWait = WaitForSingleObject(hThread, EXIT_TIMEOUT);
		if (retWait == WAIT_OBJECT_0)
		{
			cout << i++ << "번 스레드 종료" << '\n';

		}
		else
		{
			DebugBreak();
		}
		CloseHandle(hThread);
	}
	CloseHandle(_hcp);
	WSACleanup();
}

void IOCPServer::RecvPost(Session* pSession)
{
	if (pSession->bConnecting == false)
	{
		if (InterlockedDecrement16(&pSession->sessionManageInfo.refCnt) == 0)
		{
			ReleaseSession(pSession);
		}
		return;
	}

	WSABUF wsaBufs[2];
	DWORD bufCnt;
	DWORD flags = 0;
	memset(&pSession->recvOverLapped, 0, sizeof(pSession->recvOverLapped));
	int freeSize = pSession->recvBuffer.GetFreeSize();
	int directEnqueueSize = pSession->recvBuffer.DirectEnqueueSize();

	if (directEnqueueSize < freeSize)
	{
		wsaBufs[0].buf = pSession->recvBuffer.GetBackBufferPtr();
		wsaBufs[0].len = directEnqueueSize;
		wsaBufs[1].buf = pSession->recvBuffer.GetBufferPtr();
		wsaBufs[1].len = freeSize - directEnqueueSize;
		bufCnt = 2;
	}
	else
	{
		wsaBufs[0].buf = pSession->recvBuffer.GetBackBufferPtr();
		wsaBufs[0].len = directEnqueueSize;
		bufCnt = 1;
	}
	int retval = WSARecv(pSession->socket, wsaBufs, bufCnt, NULL, &flags, &pSession->recvOverLapped, NULL);
	if (retval == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		if (error != ERROR_IO_PENDING)
		{
			if (error != WSAECONNRESET && error != WSAECONNABORTED
				&& error != WSAENOTSOCK)
			{
				std::cout << "WSARecv() error : " << error << '\n';

			}
			pSession->bConnecting = false;
			if (InterlockedDecrement16(&pSession->sessionManageInfo.refCnt) == 0)
			{
				ReleaseSession(pSession);
			}
		}
	}

}

void IOCPServer::SendPost(Session* pSession)
{
	if (pSession->bConnecting == false)
	{
		return;
	}

	while (1)
	{
		if (pSession->sendBuffer.GetUseSize() == 0 || InterlockedExchange(&pSession->bSending, true) != false)
		{
			return;
		}
		if (pSession->sendBuffer.GetUseSize() > 0)
		{
			break;
		}
		else
		{
			pSession->bSending = false;
		}
	}

	WSABUF wsaBufs[MAX_SEND_BUF_CNT];
	CSerialBuffer* pBufs[MAX_SEND_BUF_CNT];
	
	int useSize = pSession->sendBuffer.GetUseSize();
	pSession->sendBufCnt=min(useSize/sizeof(CSerialBuffer*), MAX_SEND_BUF_CNT);
	pSession->sendBuffer.Peek((char*)pBufs, useSize);
	NetHeader* pNetHeader;
	for(int i=0;i< pSession->sendBufCnt;i++)
	{
		if (_bWan)
		{
			pNetHeader = pBufs[i]->GetWanHeaderPtr();
			pNetHeader->len = pBufs[i]->GetUseSize();
			wsaBufs[i].buf = (char*)pNetHeader;
			wsaBufs[i].len = pNetHeader->len + WAN_HEADER_SIZE;
		}
		else
		{
			pNetHeader = pBufs[i]->GetLanHeaderPtr();
			pNetHeader->len = pBufs[i]->GetUseSize();
			wsaBufs[i].buf = (char*)pNetHeader;
			wsaBufs[i].len = pNetHeader->len + LAN_HEADER_SIZE;
		}
	}
	


	memset(&pSession->sendOverLapped, 0, sizeof(pSession->sendOverLapped));
	
	InterlockedIncrement16(&pSession->sessionManageInfo.refCnt);
	int retval = WSASend(pSession->socket, wsaBufs, pSession->sendBufCnt, NULL, 0, &pSession->sendOverLapped, NULL);
	if (retval == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		if (error != ERROR_IO_PENDING)
		{
			if (error != WSAECONNRESET && error != WSAECONNABORTED
				&& error != WSAENOTSOCK)
			{
				std::cout << "WSASend() error : " << error << '\n';
			}
			pSession->bConnecting = false;
			if (InterlockedDecrement16(&pSession->sessionManageInfo.refCnt) == 0)
			{
				ReleaseSession(pSession);
			}
		}
	}
}

void IOCPServer::Unicast(SessionInfo sessionInfo, CSerialBuffer* pBuf)
{
	_sendCnt++;
	Session* pSession = FindSession(sessionInfo);
	if (pSession == nullptr)
	{
		return;
	}

	pBuf->IncrementRefCnt();
	AcquireSRWLockExclusive(&pSession->sessionLock);
	int enqueue_len = pSession->sendBuffer.Enqueue((char*)&pBuf, sizeof(pBuf));
	if (enqueue_len < sizeof(pBuf))
	{
		cout << "sendBufFULL" << endl;
		pSession->bConnecting = false;
	}
	ReleaseSRWLockExclusive(&pSession->sessionLock);

	SendPost(pSession);
	if (InterlockedDecrement16(&pSession->sessionManageInfo.refCnt) == 0)
	{
		ReleaseSession(pSession);
	}
	return;
}

void IOCPServer::DisConnect(SessionInfo sessionInfo)
{
	Session* pSession = FindSession(sessionInfo);
	if (pSession == nullptr)
	{
		return ;
	}
	pSession->bConnecting = false;
	CancelIo((HANDLE)pSession->socket);
	if (InterlockedDecrement16(&pSession->sessionManageInfo.refCnt) == 0)
	{
		ReleaseSession(pSession);
	}
}

unsigned __stdcall IOCPServer::AcceptThreadFunc(LPVOID arg)
{
	IOCPServer* pServer = (IOCPServer*)arg;
	pServer->AcceptWork();
	return 0;
}

unsigned __stdcall IOCPServer::IOCPWorkThreadFunc(LPVOID arg)
{
	IOCPServer* pServer = (IOCPServer*)arg;
	pServer->IOCPWork();
	return 0;
}

void IOCPServer::RecvCompletionRoutine(Session* pSession)
{
	while (1)
	{
		int useSize = pSession->recvBuffer.GetUseSize();

		if (useSize < sizeof(NetHeader))
		{
			break;
		}

		NetHeader header;

		int peekSize = pSession->recvBuffer.Peek((char*)&header, sizeof(header));
		if (peekSize != sizeof(header))
		{
			DebugBreak();
		}

		if (useSize < sizeof(NetHeader) + header.len)
		{
			break;
		}
		pSession->recvBuffer.MoveFront(sizeof(header));
		OnRecv(pSession->sessionInfo, pSession->recvBuffer);
		_recvCnt++;
	}
	RecvPost(pSession);
}

void IOCPServer::SendCompletionRoutine(Session* pSession)
{
	CSerialBuffer* pBufs[MAX_SEND_BUF_CNT];
	pSession->sendBuffer.Dequeue((char*)pBufs, pSession->sendBufCnt * sizeof(NetHeader*));
	for(int i=0;i< pSession->sendBufCnt;i++)
	{
		pBufs[i]->DecrementRefCnt();
	}
	pSession->bSending = false;
	SendPost(pSession);
	if (InterlockedDecrement16(&pSession->sessionManageInfo.refCnt) == 0)
	{
		ReleaseSession(pSession);
	}
}

void IOCPServer::IOCPWork()
{
	while (1)
	{
		DWORD cbTransferred = 0;
		Session* pSession = nullptr;

		OVERLAPPED* pOverlapped = nullptr;
		int retval = GetQueuedCompletionStatus(_hcp, &cbTransferred, (PULONG_PTR)&pSession, &pOverlapped, INFINITE);
		if (pOverlapped == nullptr)
		{
			if ((ULONG_PTR)pSession != SERVER_DOWN_KEY)
			{
				int error = WSAGetLastError();
				cout << "gqcs error : " << error << endl;
			}
			else
			{
				break;
			}
			DebugBreak();
		}
		else
		{
			if (retval == false || cbTransferred == 0)
			{
				int error;
				if (retval == false)
				{
					error = WSAGetLastError();
					if (error != ERROR_CONNECTION_ABORTED && error != ERROR_NETNAME_DELETED
						&& error != WSA_OPERATION_ABORTED)
					{
						cout << "gqcs ret false error : " << error << endl;
					}
				}
				pSession->bConnecting = false;
				if (InterlockedDecrement16(&pSession->sessionManageInfo.refCnt) == 0)
				{
					ReleaseSession(pSession);
				}
				continue;
			}

			if (&pSession->recvOverLapped == pOverlapped)
			{
				pSession->recvBuffer.MoveBack(cbTransferred);
				RecvCompletionRoutine(pSession);
			}
			else if (&pSession->sendOverLapped == pOverlapped)
			{
				SendCompletionRoutine(pSession);
			}
		}

	}
	return;
}

void IOCPServer::IOCPRun()
{
	timeBeginPeriod(1);

	int ret_bind;
	int ret_listen;
	int ret_ioctl;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		int error = WSAGetLastError();
		cout << "WSAStartup() error : " << error << '\n';
		DebugBreak();
	}

	_listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenSock == INVALID_SOCKET)
	{
		int error = WSAGetLastError();
		cout << "socket() error : " << error << '\n';
		DebugBreak();

	}
	/*int sendBufSize = 0;
	int ret_set = setsockopt(g_listenSock, SOL_SOCKET, SO_SNDBUF, (char*)&sendBufSize, sizeof(sendBufSize));
	if (ret_set == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		cout << "sendBuf 0 error : " << error << '\n';
		DebugBreak();
	}*/
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVER_PORT);
	ret_bind = bind(_listenSock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (ret_bind == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		cout << "bind() error : " << error << '\n';
		DebugBreak();
	}

	struct linger _linger;
	_linger.l_onoff = 1;
	_linger.l_linger = 0;

	int ret_linger = setsockopt(_listenSock, SOL_SOCKET, SO_LINGER, (char*)&_linger, sizeof(_linger));
	if (ret_linger == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		cout << "linger error : " << error << endl;
		DebugBreak();
	}

	ret_listen = listen(_listenSock, SOMAXCONN);
	if (ret_listen == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		cout << "listen() error : " << error << endl;
		DebugBreak();
	}
	_hcp = CreateNewCompletionPort(CONCURRENT_THREAD_NUM);
	if (_hcp == NULL)
	{
		int error = WSAGetLastError();
		cout << "CreateNewCompletionPort error : " << error << endl;
		DebugBreak();
	};

	HANDLE hAcceptThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThreadFunc, this, 0, NULL);
	if (hAcceptThread == NULL)
	{
		int error = WSAGetLastError();
		cout << "_beginthreadex error : " << error << '\n';
		DebugBreak();
	}
	_hThreadList.push_back(hAcceptThread);


	for (int i = 0; i < IOCP_THREAD_NUM; i++)
	{
		HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, IOCPWorkThreadFunc, this, 0, NULL);
		if (hThread == NULL)
		{
			int error = WSAGetLastError();
			cout << "_beginthreadex error : " << error << '\n';
			DebugBreak();
		}
		_hThreadList.push_back(hThread);
	}
	return;
}

void IOCPServer::ServerControl()
{
	static bool bControlMode = false;
	if (_kbhit())
	{
		char ControlKey = _getch();
		if ('u' == ControlKey || 'U' == ControlKey)
		{
			bControlMode = true;
			std::cout << "Control Mode : Press Q - Quit" << std::endl;
			std::cout << "Control Mode : Press L - Key Lock" << std::endl;
		}

		if (bControlMode && ('l' == ControlKey || 'L' == ControlKey))
		{
			std::cout << "Control Lock... Press U - Control Unlock" << std::endl;
			bControlMode = false;
		}

		if (bControlMode && ('q' == ControlKey || 'Q' == ControlKey))
		{
			_bShutdown = true;
		}
	}
}

int IOCPServer::GetAcceptTPS()
{
	int ret = _acceptCnt;
	_acceptCnt = 0;
	return ret;
}

int IOCPServer::GetRecvTPS()
{
	int ret = _recvCnt;
	InterlockedExchange(&_recvCnt,0);
	return ret;
}

int IOCPServer::GetSendTPS()
{
	int ret = _sendCnt;
	InterlockedExchange(&_sendCnt, 0);
	return ret;
}
