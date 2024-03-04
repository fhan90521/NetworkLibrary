#define _CRT_SECURE_NO_WARNINGS
#include "IOCPServer.h"
#include <list>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"
#include<conio.h>
#include<memory.h>
#include "NetworkHeader.h"
#include "Log.h"
#include "MyNew.h"
//#include "Log.h"
using namespace rapidjson;

HANDLE IOCPServer::CreateNewCompletionPort(DWORD dwNumberOfConcurrentThreads)
{
	return(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL,
		0, dwNumberOfConcurrentThreads));
}
BOOL IOCPServer::AssociateDeviceWithCompletionPort(HANDLE hCompletionPort, HANDLE hDevice, ULONG_PTR dwCompletionKey)
{
	HANDLE hcp = CreateIoCompletionPort(hDevice, hCompletionPort, dwCompletionKey, 0);
	return (hcp == hCompletionPort);
}

void IOCPServer::DropIoPending(SessionInfo sessionInfo)
{
	Session* pSession = FindSession(sessionInfo);
	if (pSession == nullptr)
	{
		return;
	}
	CancelIo((HANDLE)pSession->socket);
	if (InterlockedDecrement16(&pSession->sessionManageInfo.refCnt) == 0)
	{
		ReleaseSession(pSession);
	}
}

void IOCPServer::GetSeverSetValues()
{
	std::ifstream fin(_settingFileName);
	if (!fin)
	{
		Log::LogOnFile(Log::SYSTEM_LEVEL,"there is no %s\n", _settingFileName.data());
		DebugBreak();
	}
	std::string json((std::istreambuf_iterator<char>(fin)), (std::istreambuf_iterator<char>()));
	fin.close();
	Document d;
	rapidjson::ParseResult parseResult = d.Parse(json.data());
	if (!parseResult) {
		fprintf(stderr, "JSON parse error: %s (%d)",GetParseError_En(parseResult.Code()), parseResult.Offset());
		exit(EXIT_FAILURE);
	}

	IOCP_THREAD_NUM = d["IOCP_THREAD_NUM"].GetInt();
	
	CONCURRENT_THREAD_NUM = d["CONCURRENT_THREAD_NUM"].GetInt();
	
	BIND_IP = d["BIND_IP"].GetString();
	
	BIND_PORT = d["BIND_PORT"].GetInt();
	
	SESSION_MAX = d["SESSION_MAX"].GetInt();
	
	PACKET_CODE = d["PACKET_CODE"].GetInt();
	
	PACKET_KEY = d["PACKET_KEY"].GetInt();
	
	LOG_LEVEL = d["LOG_LEVEL"].GetInt();
	return;
}

void IOCPServer::ServerSetting()
{
	GetSeverSetValues();
	
	timeBeginPeriod(1);
	int ret_bind;
	WSADATA wsa;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		int error = WSAGetLastError();
		Log::LogOnFile(Log::SYSTEM_LEVEL, "WSAStartup() error : %d\n", error);
		DebugBreak();
	}

	_listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenSock == INVALID_SOCKET)
	{
		int error = WSAGetLastError();
		Log::LogOnFile(Log::SYSTEM_LEVEL, "socket() error : %d\n", error);
		DebugBreak();

	}

	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(BIND_PORT);
	ret_bind = bind(_listenSock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (ret_bind == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		Log::LogOnFile(Log::SYSTEM_LEVEL, "bind() error : %d\n", error);
		DebugBreak();
	}

	/*int sendBufSize = 0;
	int ret_set = setsockopt(_listenSock, SOL_SOCKET, SO_SNDBUF, (char*)&sendBufSize, sizeof(sendBufSize));
	if (ret_set == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		Log::LogOnFile(Log::SYSTEM_LEVEL, "sendbuf 0 error : %d\n", error);
		DebugBreak();
	}*/

	struct linger _linger;
	_linger.l_onoff = 1;
	_linger.l_linger = 0;

	int ret_linger = setsockopt(_listenSock, SOL_SOCKET, SO_LINGER, (char*)&_linger, sizeof(_linger));
	if (ret_linger == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		Log::LogOnFile(Log::SYSTEM_LEVEL, "linger error : %d", error);
		DebugBreak();
	}

	_sessionArray = new Session[SESSION_MAX];
	for (int i = 0; i < SESSION_MAX; i++)
	{
		_validIndexStack.Push(i);
	}
	if (_bWan)
	{
		WanHeader::SetConstKey(PACKET_KEY);
		WanHeader::SetNetCode(PACKET_CODE);
	}
	Log::SetLogLevel(LOG_LEVEL);

	_hcp = CreateNewCompletionPort(CONCURRENT_THREAD_NUM);
	if (_hcp == NULL)
	{
		int error = WSAGetLastError();
		Log::LogOnFile(Log::SYSTEM_LEVEL, "CreateNewCompletionPort error : %d", error);
		DebugBreak();
	};
	InitializeSRWLock(&_pRoomsLock);
	_pRooms = new Room*[MAX_ROOM_CNT];
	_hReserveDisconnectEvent=CreateEvent(NULL, false, false, NULL);
	_hShutDownEvent = CreateEvent(NULL, false, false, NULL);
}

Session* IOCPServer::FindSession(SessionInfo sessionInfo)
{
	Session* pSession = &_sessionArray[sessionInfo.index.val];
	InterlockedIncrement16(&pSession->sessionManageInfo.refCnt);
	if (pSession->sessionManageInfo.bDeallocated == true)
	{
		//Release가 이미 되고 있다. 이미되고 할당 받는중인데 Release가 되는 문제 <-문제 a -> 받는중이라도 bDeallocated==false일 수 없어서 release 될 일 없다.
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

	unsigned short index; 
	bool retPop = _validIndexStack.Pop(&index);
	if (retPop == false)
	{
		Log::LogOnFile(Log::SYSTEM_LEVEL,"_validIndexStack Pop error\n");
		DebugBreak();
	}

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
	CSendBuffer* pBuf;
	while(pSession->sendBufQ.Dequeue(&pBuf))
	{
		pBuf->DecrementRefCnt();
	}
	for (int i = 0; i < pSession->sendBufCnt; i++)
	{
		//OnSend(pSession->sessionInfo, pSession->pSendedBufArr[i]->GetPayLoadSize());
		(pSession->pSendedBufArr[i])->DecrementRefCnt();
	}
	pSession->sendBufCnt = 0;
	pSession->bSending = false;
	pSession->bReservedDisconnect = false;
	pSession->recvBuffer.ClearBuffer();
	InterlockedExchange8(&pSession->onConnecting, true);
	_validIndexStack.Push(pSession->sessionInfo.index.val);
	OnDisconnect(sessionInfo);
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
		if (_bShutdown)
		{
			break;
		}
		clientSock = accept(_listenSock, (struct sockaddr*)&clientaddr, &addrlen);
		if (clientSock == INVALID_SOCKET)
		{
			int error = WSAGetLastError();
			if (error != WSAEINTR)
			{
				Log::LogOnFile(Log::SYSTEM_LEVEL, "accept error : %d\n", error);
			}
			continue;
		}
		else
		{
			//Log용
			_acceptCnt++;
		}
		if (_validIndexStack.Size()<=0)
		{
			closesocket(clientSock);
			continue;
		}


		inet_ntop(AF_INET, &clientaddr.sin_addr, ip, sizeof(ip));
		port = ntohs(clientaddr.sin_port);
		if (OnAcceptRequest(ip, port) == false)
		{
			continue;
		}

		Session* pSession = AllocSession(clientSock);
		

		bool ret = AssociateDeviceWithCompletionPort(_hcp, (HANDLE)clientSock, (ULONG_PTR)pSession);
		if (ret == false)
		{
			Log::LogOnFile(Log::SYSTEM_LEVEL, "AssociateDeviceWithCompletionPort error : %d\n", WSAGetLastError());
			DebugBreak();
		}

		pSession->socket = clientSock;
		strcpy_s(pSession->ip, INET_ADDRSTRLEN, ip);
		pSession->port = port;
		OnAccept(pSession->sessionInfo);
		RecvPost(pSession);
	}
	return;
}

void IOCPServer::CloseServer()
{
	closesocket(_listenSock);
	for (int i = 0; i < IOCP_THREAD_NUM; i++)
	{
		PostQueuedCompletionStatus(_hcp, SERVER_DOWN, SERVER_DOWN,(LPOVERLAPPED)SERVER_DOWN);
	}

	int i = 0;
	for (HANDLE hThread : _hThreadList)
	{
		DWORD retWait = WaitForSingleObject(hThread, EXIT_TIMEOUT);
		if (retWait == WAIT_OBJECT_0)
		{
			Log::LogOnFile(Log::SYSTEM_LEVEL, "%d 스레드 종료\n", GetThreadId(hThread));
		}
		else
		{
			Log::LogOnFile(Log::SYSTEM_LEVEL, "%d 스레드 종료 TimeOut\n", GetThreadId(hThread));
		}
		CloseHandle(hThread);
	}

	for (int i = 0; i < SESSION_MAX; i++)
	{
		closesocket(_sessionArray[i].socket);
	}
	CloseHandle(_hcp);
	CloseHandle(_hReserveDisconnectEvent);
	CloseHandle(_hShutDownEvent);
	WSACleanup();
	delete[] _sessionArray;
	delete[] _pRooms;
}

void IOCPServer::RecvPost(Session* pSession)
{
	if (pSession->onConnecting == false)
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

	SessionInfo sessionInfo = pSession->sessionInfo;

	int retval = WSARecv(pSession->socket, wsaBufs, bufCnt, NULL, &flags, &pSession->recvOverLapped, NULL);
	if (retval == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		if (error != ERROR_IO_PENDING)
		{
			if (error != WSAECONNRESET && error != WSAECONNABORTED
				&& error != WSAENOTSOCK && error != WSAESHUTDOWN)
			{
				Log::LogOnFile(Log::SYSTEM_LEVEL, "WSARecv() error: %d\n", error);
			}
			if (InterlockedDecrement16(&pSession->sessionManageInfo.refCnt) == 0)
			{
				ReleaseSession(pSession);
			}
		}
		else
		{
			if (pSession->onConnecting == false)
			{
				DropIoPending(sessionInfo);
			}
		}
	}

}

bool IOCPServer::GetSendAuthority(Session* pSession)
{
	while (1)
	{
		if (pSession->sendBufQ.Size() == 0 || InterlockedExchange8(&pSession->bSending, true) != false)
		{
			return false;
		}
		if (pSession->sendBufQ.Size() > 0)
		{
			return true;;
		}
		else
		{
			pSession->bSending = false;
		}
	}
}

void IOCPServer::SendPost(Session* pSession)
{
	WSABUF wsaBufs[MAX_SEND_BUF_CNT];
	pSession->sendBufCnt = min(MAX_SEND_BUF_CNT, pSession->sendBufQ.Size());
	for (int i = 0; i < pSession->sendBufCnt; i++)
	{
		pSession->sendBufQ.Dequeue(&(pSession->pSendedBufArr[i]));
		if (_bWan)
		{
			wsaBufs[i].buf = (char*)(pSession->pSendedBufArr[i])->GetWanHeader();
			wsaBufs[i].len = (pSession->pSendedBufArr[i])->GetPayLoadSize()+sizeof(WanHeader);
		}
		else
		{
			wsaBufs[i].buf = (char*)(pSession->pSendedBufArr[i])->GetLanHeader();
			wsaBufs[i].len = (pSession->pSendedBufArr[i])->GetPayLoadSize() + sizeof(LanHeader);
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
				&& error != WSAENOTSOCK&&error!= WSAESHUTDOWN)
			{
				Log::LogOnFile(Log::SYSTEM_LEVEL, "WSASend() error: %d\n", error);
			}
			//Disconnect(pSession);
			if (InterlockedDecrement16(&pSession->sessionManageInfo.refCnt) == 0)
			{
				ReleaseSession(pSession);
			}
		}
	}
}

void IOCPServer::RequestSend(Session* pSession)
{
	InterlockedIncrement16(&pSession->sessionManageInfo.refCnt);
	bool retPQCS =PostQueuedCompletionStatus(_hcp, REQUEST_SEND, (ULONG_PTR)pSession, (LPOVERLAPPED)REQUEST_SEND);
	if (retPQCS==false)
	{
		Log::LogOnFile(Log::SYSTEM_LEVEL, "RequestSend error: %d\n", WSAGetLastError());
	}
}

void IOCPServer::Unicast(SessionInfo sessionInfo, CSendBuffer* pBuf, bool bDisconnectAfterSend)
{
	Session* pSession = FindSession(sessionInfo);
	if (pSession == nullptr)
	{
		return;
	}

	if (pSession->bReservedDisconnect == true)
	{
		if (InterlockedDecrement16(&pSession->sessionManageInfo.refCnt) == 0)
		{
			ReleaseSession(pSession);
		}
		return;
	}


	if (pSession->sendBufQ.Size() < SENDQ_MAX_LEN)
	{
		if (_bWan)
		{
			pBuf->SetWanHeader();
		}
		else
		{
			pBuf->SetLanHeader();
		}
		pBuf->IncrementRefCnt();
		pSession->sendBufQ.Enqueue(pBuf);
	}
	else
	{
		InterlockedExchange8(&pSession->onConnecting, false);
		CancelIo((HANDLE)pSession->socket);
	}

	if (bDisconnectAfterSend)
	{
		InterlockedExchange8(&pSession->bReservedDisconnect, true);
	}

	if (GetSendAuthority(pSession) == true)
	{
		RequestSend(pSession);
	}

	if (InterlockedDecrement16(&pSession->sessionManageInfo.refCnt) == 0)
	{
		ReleaseSession(pSession);
	}
	return;
}

void IOCPServer::Disconnect(SessionInfo sessionInfo)
{
	Session* pSession = FindSession(sessionInfo);
	if (pSession == nullptr)
	{
		return ;
	}
	InterlockedExchange8(&pSession->onConnecting, false);
	CancelIoEx((HANDLE)pSession->socket,NULL);
	if (InterlockedDecrement16(&pSession->sessionManageInfo.refCnt) == 0)
	{
		ReleaseSession(pSession);
	}
}



bool IOCPServer::GetClientIp(SessionInfo sessionInfo,String& outPar)
{
	Session* pSession = FindSession(sessionInfo);
	if (pSession == nullptr)
	{
		return false;
	}
	outPar = pSession->ip;
	if (InterlockedDecrement16(&pSession->sessionManageInfo.refCnt) == 0)
	{
		ReleaseSession(pSession);
	}
	return true;
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

void IOCPServer::RegisterThread(_beginthreadex_proc_type pFunction)
{
	HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, pFunction, this, 0, NULL);
	if (hThread == NULL)
	{
		Log::LogOnFile(Log::SYSTEM_LEVEL, "_beginthreadex error: %d\n", WSAGetLastError());
		DebugBreak();
	}
	_hThreadList.push_back(hThread);
}

template<typename NetHeader>
void IOCPServer::RecvCompletionRoutine(Session* pSession)
{
	LONG recvCnt = 0;
	while (1)
	{
		int useSize = pSession->recvBuffer.GetUseSize();

		if (useSize < sizeof(NetHeader))
		{
			break;
		}
		NetHeader netHeader;
		int peekSize = pSession->recvBuffer.Peek((char*)&netHeader, sizeof(netHeader));
		if (peekSize != sizeof(netHeader))
		{
			Log::LogOnFile(Log::SYSTEM_LEVEL, "recvBuffer Peek Size error 종료\n");
			DebugBreak();
		}
		if constexpr (std::is_same<NetHeader, WanHeader>::value)
		{
			if (netHeader.code != WanHeader::NetCode)
			{
				InterlockedExchange8(&pSession->onConnecting, false);
				break;
			}
		}
		if (netHeader.len > PAYLOAD_MAX_LEN)
		{
			InterlockedExchange8(&pSession->onConnecting, false);
			break;
		}
		if (useSize < sizeof(NetHeader) + netHeader.len)
		{
			break;
		}
		pSession->recvBuffer.MoveFront(sizeof(netHeader));
		CRecvBuffer buf(&pSession->recvBuffer, netHeader.len);
		if constexpr( std::is_same<NetHeader,WanHeader>::value)
		{
			if (buf.Decode(&netHeader) == false)
			{
				InterlockedExchange8(&pSession->onConnecting, false);
				break;
			}
		}
		OnRecv(pSession->sessionInfo, buf);
		recvCnt++;
	}
	InterlockedAdd(&_recvCnt, recvCnt);
	RecvPost(pSession);
}

void IOCPServer::SendCompletionRoutine(Session* pSession)
{
	InterlockedAdd(&_sendCnt, pSession->sendBufCnt);
	for(int i=0;i< pSession->sendBufCnt;i++)
	{
		//OnSend(pSession->sessionInfo, pSession->pSendedBufArr[i]->GetPayLoadSize());
		(pSession->pSendedBufArr[i])->DecrementRefCnt();
	}
	pSession->sendBufCnt = 0;
	pSession->bSending = false;
	if (pSession->bReservedDisconnect == true)
	{
		if (pSession->sendBufQ.Size() > 0)
		{
			if (GetSendAuthority(pSession) == true)
			{
				SendPost(pSession);
			}
		}
		else
		{
			ULONG64 currentTime = GetTickCount64();
			ReserveInfo reserveInfo;
			reserveInfo.reserveTime = currentTime + RESERVE_DISCONNECT_MS;
			reserveInfo.sessionInfo = pSession->sessionInfo;
			_reserveDisconnectQ.Enqueue(reserveInfo);
			SetEvent(_hReserveDisconnectEvent);
		}
	}
	else
	{
		if (GetSendAuthority(pSession) == true)
		{
			SendPost(pSession);
		}
	}

	if (InterlockedDecrement16(&pSession->sessionManageInfo.refCnt) == 0)
	{
		ReleaseSession(pSession);
	}
}

void IOCPServer::RequestSendCompletionRoutine(Session* pSession)
{
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
		int error;

		OVERLAPPED* pOverlapped = nullptr;
		int retval = GetQueuedCompletionStatus(_hcp, &cbTransferred, (PULONG_PTR)&pSession, &pOverlapped, INFINITE);
		if (pOverlapped == nullptr)
		{
			Log::LogOnFile(Log::SYSTEM_LEVEL, "GQCS pOverlapped null error: %d\n",WSAGetLastError());
			DebugBreak();
		}
		else
		{
			if (retval == false || cbTransferred == 0)
			{
				if (retval == false)
				{
					error = WSAGetLastError();
					if (error != ERROR_CONNECTION_ABORTED && error != ERROR_NETNAME_DELETED
						&& error != WSA_OPERATION_ABORTED)
					{
						Log::LogOnFile(Log::SYSTEM_LEVEL, "gqcs ret falseerror: %d\n", error);
					}
				}
				//Disconnect(pSession);
				if (InterlockedDecrement16(&pSession->sessionManageInfo.refCnt) == 0)
				{
					ReleaseSession(pSession);
				}
				continue;
			}

			if (pOverlapped  == &pSession->recvOverLapped )
			{
				pSession->recvBuffer.MoveBack(cbTransferred);
				if (_bWan)
				{
					RecvCompletionRoutine<WanHeader>(pSession);
				}
				else
				{
					RecvCompletionRoutine<LanHeader>(pSession);
				}
			}
			else if (pOverlapped  == &pSession->sendOverLapped )
			{
				SendCompletionRoutine(pSession);
			}
			else if (pOverlapped == (LPOVERLAPPED)REQUEST_SEND)
			{
				RequestSendCompletionRoutine(pSession);
			}
			else if (pOverlapped == (LPOVERLAPPED)SERVER_DOWN)
			{
				break;
			}
			else
			{
				error = WSAGetLastError();
				Log::LogOnFile(Log::SYSTEM_LEVEL, "pOverlapped  error: %d\n", error);
				DebugBreak();
			}
		}

	}
	return;
}
void IOCPServer::IOCPRun()
{
	int ret_listen;
	ret_listen = listen(_listenSock, SOMAXCONN);
	if (ret_listen == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		Log::LogOnFile(Log::SYSTEM_LEVEL, "listen() error : %d", error);
		DebugBreak();
	}
	for (int i = 0; i < IOCP_THREAD_NUM; i++)
	{
		RegisterThread(IOCPServer::IOCPWorkThreadFunc);
	}
	for (int i = 0; i < ROOM_WORK_THREAD_CNT; i++)
	{
		RegisterThread(IOCPServer::RoomWorkThreadFunc);
	}
	RegisterThread(IOCPServer::AcceptThreadFunc);
	RegisterThread(IOCPServer::ReserveDisconnectManageThreadFunc);
	return;
}
bool IOCPServer::ServerControl()
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
			SetEvent(_hShutDownEvent);
		}
	}
	return bControlMode;
}
int IOCPServer::GetAcceptCnt()
{
	int ret = _acceptCnt;
	_acceptCnt = 0;
	return ret;
}
int IOCPServer::GetRecvCnt()
{
	int ret = _recvCnt;
	InterlockedExchange(&_recvCnt,0);
	return ret;
}
int IOCPServer::GetSendCnt()
{
	int ret = _sendCnt;
	InterlockedExchange(&_sendCnt, 0);
	return ret;
}

int IOCPServer::GetConnectingSessionCnt()
{
	return SESSION_MAX-_validIndexStack.Size();
}

void IOCPServer::SetMaxPayloadLen(int len)
{
	PAYLOAD_MAX_LEN = len;
	return;
}

void IOCPServer::RoomWork()
{
	if (MS_PER_ROOM_FRAME == -1)
	{
		return;
	}
	while (1)
	{
		if (_bShutdown)
		{
			break;
		}
		AcquireSRWLockShared(&_pRoomsLock);
		for (int i = 0; i< _registeredRoomCnt; i++)
		{
			_pRooms[i]->ProcessRoom();
		}
		ReleaseSRWLockShared(&_pRoomsLock);
	}
}

unsigned __stdcall IOCPServer::RoomWorkThreadFunc(LPVOID arg)
{
	IOCPServer* pServer = (IOCPServer*)arg;
	pServer->RoomWork();
	return 0;
}


bool IOCPServer::RegisterRoom(Room* pRoom)
{
	bool ret = false;;
	AcquireSRWLockExclusive(&_pRoomsLock);
	if (_pRoomSet.size() < MAX_ROOM_CNT)
	{
		auto retInsert = _pRoomSet.insert(pRoom);
		if (retInsert.second == true)
		{
			ret = true;
			_pRooms[_registeredRoomCnt++] = pRoom;
		}
	}
	ReleaseSRWLockExclusive(&_pRoomsLock);
	return ret;
}

bool IOCPServer::DeregisterRoom(Room* pRoom)
{
	AcquireSRWLockExclusive(&_pRoomsLock);
	size_t retErase = _pRoomSet.erase(pRoom);
	if (retErase == 1)
	{
		for (int i = 0; i < _registeredRoomCnt; i++)
		{
			if (_pRooms[i] == pRoom)
			{
				_pRooms[i] = _pRooms[--_registeredRoomCnt];
			}
		}
	}
	ReleaseSRWLockExclusive(&_pRoomsLock);
	return retErase;
}


void IOCPServer::ReserveDisconnectManage()
{
	HANDLE _hArr[2] = { _hReserveDisconnectEvent,_hShutDownEvent };
	while (1)
	{
	
		DWORD retWait = WaitForMultipleObjects(2, _hArr, false, INFINITE);
		if (retWait == WAIT_OBJECT_0)
		{
			while (1)
			{
				if ((_reserveDisconnectQ.Size() == 0 && _reserveDisconnectList.size() == 0)|| _bShutdown)
				{
					break;
				}
				while(_reserveDisconnectQ.Size() > 0)
				{
					ReserveInfo reserveInfo;
					_reserveDisconnectQ.Dequeue(&reserveInfo);
					_reserveDisconnectList.push_back(reserveInfo);
				}
				ULONG64 currentTime = GetTickCount64();
				for(auto itReserveInfo = _reserveDisconnectList.begin(); itReserveInfo !=_reserveDisconnectList.end();)
				{

					if (currentTime > itReserveInfo->reserveTime)
					{
						Disconnect(itReserveInfo->sessionInfo);
						itReserveInfo = _reserveDisconnectList.erase(itReserveInfo);
					}
					else
					{
						itReserveInfo++;
					}
				}
				Sleep(RESERVE_DISCONNECT_MS);
			}
		}
		else
		{
			break;
		}

	}
}

unsigned __stdcall IOCPServer::ReserveDisconnectManageThreadFunc(LPVOID arg)
{
	IOCPServer* pServer = (IOCPServer*)arg;
	pServer->ReserveDisconnectManage();
	return 0;
}
