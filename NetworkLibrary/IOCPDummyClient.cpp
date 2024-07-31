#define _CRT_SECURE_NO_WARNINGS
#include "IOCPDummyClient.h"
#include <list>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include<conio.h>
#include<memory.h>
#include "NetworkHeader.h"
#include "Log.h"
#include "ParseJson.h"
//#include "Log.h"

HANDLE IOCPDummyClient::CreateNewCompletionPort(DWORD dwNumberOfConcurrentThreads)
{
	return(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL,
		0, dwNumberOfConcurrentThreads));
}
BOOL IOCPDummyClient::AssociateDeviceWithCompletionPort(HANDLE hCompletionPort, HANDLE hDevice, ULONG_PTR dwCompletionKey)
{
	HANDLE hcp = CreateIoCompletionPort(hDevice, hCompletionPort, dwCompletionKey, 0);
	return (hcp == hCompletionPort);
}

void IOCPDummyClient::DropIoPending(SessionInfo sessionInfo)
{
	Session* pSession = FindSession(sessionInfo);
	if (pSession == nullptr)
	{
		return;
	}
	CancelIoEx((HANDLE)pSession->socket,NULL);
	if (InterlockedDecrement16(&pSession->sessionManageInfo.refCnt) == 0)
	{
		ReleaseSession(pSession);
	}
}

void IOCPDummyClient::GetDummySetValues(std::string settingFileName)
{
	Document dummySetValues = ParseJson(settingFileName);

	IOCP_THREAD_NUM = dummySetValues["IOCP_THREAD_NUM"].GetInt();
	
	CONCURRENT_THREAD_NUM = dummySetValues["CONCURRENT_THREAD_NUM"].GetInt();
	
	SERVER_IP = dummySetValues["SERVER_IP"].GetString();
	
	SERVER_PORT = dummySetValues["SERVER_PORT"].GetInt();
	
	SESSION_MAX = dummySetValues["SESSION_MAX"].GetInt();
	
	PACKET_CODE = dummySetValues["PACKET_CODE"].GetInt();
	
	PACKET_KEY = dummySetValues["PACKET_KEY"].GetInt();
	
	LOG_LEVEL = dummySetValues["LOG_LEVEL"].GetInt();
	return;
}

void IOCPDummyClient::DummySetting()
{	
	timeBeginPeriod(1);
	WSADATA wsa;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		int error = WSAGetLastError();
		Log::LogOnFile(Log::SYSTEM_LEVEL, "WSAStartup() error : %d\n", error);
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
	_hReserveDisconnectEvent=CreateEvent(NULL, false, false, NULL);
	_hShutDownEvent = CreateEvent(NULL, false, false, NULL);

}

Session* IOCPDummyClient::FindSession(SessionInfo sessionInfo)
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

Session* IOCPDummyClient::AllocSession(SOCKET serverSock)
{

	USHORT index; 
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

	bool ret = AssociateDeviceWithCompletionPort(_hcp, (HANDLE)serverSock, (ULONG_PTR)pSession);
	if (ret == false)
	{
		Log::LogOnFile(Log::SYSTEM_LEVEL, "AssociateDeviceWithCompletionPort error : %d\n", WSAGetLastError());
		DebugBreak();
	}
	pSession->socket = serverSock;
	InterlockedExchange8(&pSession->onConnecting, true);
	return pSession;
}

void IOCPDummyClient::ReleaseSession(Session* pSession)
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
	_validIndexStack.Push(pSession->sessionInfo.index.val);
	OnDisconnect(sessionInfo);
}

bool IOCPDummyClient::Connect()
{
	if (_validIndexStack.Size() <= 0)
	{
		return false;
	}
	SOCKET serverSock = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	inet_pton(AF_INET, SERVER_IP.data(), &serverAddr.sin_addr);
	serverAddr.sin_port = htons(SERVER_PORT);
	int retConnect = connect(serverSock, (sockaddr*)&serverAddr, sizeof(serverAddr));
	if (retConnect == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		Log::LogOnFile(Log::SYSTEM_LEVEL, "connect fail: %d", error);
		return false;
	}

	struct linger _linger;
	_linger.l_onoff = 1;
	_linger.l_linger = 0;

	int ret_linger = setsockopt(serverSock, SOL_SOCKET, SO_LINGER, (char*)&_linger, sizeof(_linger));
	if (ret_linger == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		Log::LogOnFile(Log::SYSTEM_LEVEL, "linger error : %d", error);
		DebugBreak();
	}
	Session* pSession = AllocSession(serverSock);
	OnConnect(pSession->sessionInfo);
	RecvPost(pSession);
	return true;
}

void IOCPDummyClient::CloseDummy()
{
	for (int i = 0; i < IOCP_THREAD_NUM; i++)
	{
		PostQueuedCompletionStatus(_hcp, DUMMY_DOWN, DUMMY_DOWN,(LPOVERLAPPED)DUMMY_DOWN);
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
}

void IOCPDummyClient::RecvPost(Session* pSession)
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

bool IOCPDummyClient::GetSendAuthority(Session* pSession)
{
	while (1)
	{
		//Size체크는 필수적인것이고 bSending flag한번체크하는 것은 인터락함수 호출을 조금 줄일 수 있을까 해서 넣은 것
		if (pSession->sendBufQ.Size() ==0 || pSession->bSending == true || InterlockedExchange8(&pSession->bSending, true) != false)
		{
			return false;
		}
		if (pSession->sendBufQ.Size() > 0)
		{
			return true;
		}
		else
		{
			InterlockedExchange8(&pSession->bSending, false);
		}
	}
}

void IOCPDummyClient::SendPost(Session* pSession)
{
	if (pSession->onConnecting == false)
	{
		return;
	}
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

void IOCPDummyClient::RequestSend(Session* pSession)
{
	InterlockedIncrement16(&pSession->sessionManageInfo.refCnt);
	bool retPQCS =PostQueuedCompletionStatus(_hcp, REQUEST_SEND, (ULONG_PTR)pSession, (LPOVERLAPPED)REQUEST_SEND);
	if (retPQCS==false)
	{
		Log::LogOnFile(Log::SYSTEM_LEVEL, "RequestSend error: %d\n", WSAGetLastError());
	}
}

void IOCPDummyClient::Unicast(SessionInfo sessionInfo, CSendBuffer* pBuf, bool bDisconnectAfterSend)
{
	Session* pSession = FindSession(sessionInfo);
	if (pSession == nullptr)
	{
		return;
	}

	if (pSession->onConnecting == false || pSession->bReservedDisconnect == true)
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
		CancelIoEx((HANDLE)pSession->socket,NULL);
		if (InterlockedDecrement16(&pSession->sessionManageInfo.refCnt) == 0)
		{
			ReleaseSession(pSession);
		}
		return;
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

void IOCPDummyClient::Disconnect(SessionInfo sessionInfo)
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

unsigned __stdcall IOCPDummyClient::IOCPWorkThreadFunc(LPVOID arg)
{
	IOCPDummyClient* pServer = (IOCPDummyClient*)arg;
	pServer->IOCPWork();
	return 0;
}

void IOCPDummyClient::CreateThread(_beginthreadex_proc_type pFunction)
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
void IOCPDummyClient::RecvCompletionRoutine(Session* pSession)
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

void IOCPDummyClient::SendCompletionRoutine(Session* pSession)
{
	InterlockedAdd(&_sendCnt, pSession->sendBufCnt);
	for(int i=0;i< pSession->sendBufCnt;i++)
	{
		//OnSend(pSession->sessionInfo, pSession->pSendedBufArr[i]->GetPayLoadSize());
		(pSession->pSendedBufArr[i])->DecrementRefCnt();
	}
	pSession->sendBufCnt = 0;
	InterlockedExchange8(&pSession->bSending, false);
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

void IOCPDummyClient::RequestSendCompletionRoutine(Session* pSession)
{
	SendPost(pSession);
	if (InterlockedDecrement16(&pSession->sessionManageInfo.refCnt) == 0)
	{
		ReleaseSession(pSession);
	}
}
void IOCPDummyClient::IOCPWork()
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
			else if (pOverlapped == (LPOVERLAPPED)DUMMY_DOWN)
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
void IOCPDummyClient::IOCPRun()
{
	for (int i = 0; i < IOCP_THREAD_NUM; i++)
	{
		CreateThread(IOCPDummyClient::IOCPWorkThreadFunc);
	}
	CreateThread(IOCPDummyClient::ReserveDisconnectManageThreadFunc);
	return;
}
bool IOCPDummyClient::DummyControl()
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
int IOCPDummyClient::GetAcceptCnt()
{
	int ret = _acceptCnt;
	_acceptCnt = 0;
	return ret;
}
int IOCPDummyClient::GetRecvCnt()
{
	int ret = _recvCnt;
	InterlockedExchange(&_recvCnt,0);
	return ret;
}
int IOCPDummyClient::GetSendCnt()
{
	int ret = _sendCnt;
	InterlockedExchange(&_sendCnt, 0);
	return ret;
}

int IOCPDummyClient::GetConnectingSessionCnt()
{
	return SESSION_MAX-_validIndexStack.Size();
}

void IOCPDummyClient::SetMaxPayloadLen(int len)
{
	PAYLOAD_MAX_LEN = len;
	return;
}
void IOCPDummyClient::ReserveDisconnectManage()
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
unsigned __stdcall IOCPDummyClient::ReserveDisconnectManageThreadFunc(LPVOID arg)
{
	IOCPDummyClient* pServer = (IOCPDummyClient*)arg;
	pServer->ReserveDisconnectManage();
	return 0;
}


