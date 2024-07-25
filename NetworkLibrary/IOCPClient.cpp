#define _CRT_SECURE_NO_WARNINGS
#include "IOCPClient.h"
#include <list>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include<conio.h>
#include<memory.h>
#include "NetworkHeader.h"
#include "Log.h"
#include "MyNew.h"
#include "ParseJson.h"
//#include "Log.h"

HANDLE IOCPClient::CreateNewCompletionPort(DWORD dwNumberOfConcurrentThreads)
{
	return(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL,
		0, dwNumberOfConcurrentThreads));
}
BOOL IOCPClient::AssociateDeviceWithCompletionPort(HANDLE hCompletionPort, HANDLE hDevice, ULONG_PTR dwCompletionKey)
{
	HANDLE hcp = CreateIoCompletionPort(hDevice, hCompletionPort, dwCompletionKey, 0);
	return (hcp == hCompletionPort);
}
void IOCPClient::DropIoPending()
{

	if (IsSessionAvailable()==false)
	{
		return;
	}
	CancelIo((HANDLE)_session.socket);
	if (InterlockedDecrement16(&_session.sessionManageInfo.refCnt) == 0)
	{
		ReleaseSession();
	}
}
void IOCPClient::GetClientSetValues(std::string settingFileName)
{
	Document clientSetValues = ParseJson(settingFileName);
	IOCP_THREAD_NUM = clientSetValues["IOCP_THREAD_NUM"].GetInt();
	CONCURRENT_THREAD_NUM = clientSetValues["CONCURRENT_THREAD_NUM"].GetInt();
	SERVER_IP = clientSetValues["SERVER_IP"].GetString();
	SERVER_PORT = clientSetValues["SERVER_PORT"].GetInt();
	PACKET_CODE = clientSetValues["PACKET_CODE"].GetInt();
	PACKET_KEY = clientSetValues["PACKET_KEY"].GetInt();
	LOG_LEVEL = clientSetValues["LOG_LEVEL"].GetInt();
	return;
}
void IOCPClient::ClientSetting()
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
}
bool IOCPClient::IsSessionAvailable()
{
	InterlockedIncrement16(&_session.sessionManageInfo.refCnt);
	if (_session.sessionManageInfo.bDeallocated == true)
	{
		//Release가 이미 되고 있다. 이미되고 할당 받는중인데 Release가 되는 문제 <-문제 a -> 받는중이라도 bDeallocated==false일 수 없어서 release 될 일 없다.
		if (InterlockedDecrement16(&_session.sessionManageInfo.refCnt) == 0)
		{
			ReleaseSession();
		}
		return false;
	}
	return true;
}
void  IOCPClient::InitializeSession(SOCKET clientSock)
{
	InterlockedIncrement16(&_session.sessionManageInfo.refCnt);
	//여기까지 bDeallocated == true이다
	_session.sessionManageInfo.bDeallocated = false;
	bool ret = AssociateDeviceWithCompletionPort(_hcp, (HANDLE)clientSock, (ULONG_PTR)&_session);
	if (ret == false)
	{
		Log::LogOnFile(Log::SYSTEM_LEVEL, "AssociateDeviceWithCompletionPort error : %d\n", WSAGetLastError());
		DebugBreak();
	}
	_session.socket = clientSock;
	_session.sendBufCnt = 0;
	_session.bSending = false;
	_session.bReservedDisconnect = false;
	_session.recvBuffer.ClearBuffer();
	InterlockedExchange8(&_session.onConnecting, true);
}
void IOCPClient::ReleaseSession()
{
	if (InterlockedCompareExchange((LONG*)&_session.sessionManageInfo, true, 0) != 0)
	{
		// refCnt ==0 && bDeallocated == false 이 아닌 경우
		return;
	}
	SessionInfo sessionInfo = _session.sessionInfo;
	closesocket(_session.socket);
	CSendBuffer* pBuf;
	while (_session.sendBufQ.Dequeue(&pBuf))
	{
		pBuf->DecrementRefCnt();
	}
	for (int i = 0; i < _session.sendBufCnt; i++)
	{
		//OnSend(pSession->sessionInfo, pSession->pSendedBufArr[i]->GetPayLoadSize());
		(_session.pSendedBufArr[i])->DecrementRefCnt();
	}
	OnDisconnect();
}
void IOCPClient::CloseClient()
{
	closesocket(_session.socket);
	for (int i = 0; i < IOCP_THREAD_NUM; i++)
	{
		PostQueuedCompletionStatus(_hcp, CLIENT_DOWN, CLIENT_DOWN, (LPOVERLAPPED)CLIENT_DOWN);
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
	CloseHandle(_hcp);
	WSACleanup();
}
void IOCPClient::RecvPost()
{
	if (_session.onConnecting == false)
	{
		if (InterlockedDecrement16(&_session.sessionManageInfo.refCnt) == 0)
		{
			ReleaseSession();
		}
		return;
	}
	WSABUF wsaBufs[2];
	DWORD bufCnt;
	DWORD flags = 0;
	memset(&_session.recvOverLapped, 0, sizeof(_session.recvOverLapped));
	int freeSize = _session.recvBuffer.GetFreeSize();
	int directEnqueueSize = _session.recvBuffer.DirectEnqueueSize();

	if (directEnqueueSize < freeSize)
	{
		wsaBufs[0].buf = _session.recvBuffer.GetBackBufferPtr();
		wsaBufs[0].len = directEnqueueSize;
		wsaBufs[1].buf = _session.recvBuffer.GetBufferPtr();
		wsaBufs[1].len = freeSize - directEnqueueSize;
		bufCnt = 2;
	}
	else
	{
		wsaBufs[0].buf = _session.recvBuffer.GetBackBufferPtr();
		wsaBufs[0].len = directEnqueueSize;
		bufCnt = 1;
	}
	SessionInfo sessionInfo = _session.sessionInfo;
	int retval = WSARecv(_session.socket, wsaBufs, bufCnt, NULL, &flags, &_session.recvOverLapped, NULL);
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
			if (InterlockedDecrement16(&_session.sessionManageInfo.refCnt) == 0)
			{
				ReleaseSession();
			}
		}
		else
		{
			if (_session.onConnecting == false)
			{
				DropIoPending();
			}
		}
	}

}
bool IOCPClient::GetSendAuthority()
{
	while (1)
	{
		if (_session.sendBufQ.Size() == 0 || _session.bSending == true || InterlockedExchange8(&_session.bSending, true) != false)
		{
			return false;
		}
		if (_session.sendBufQ.Size() > 0)
		{
			return true;;
		}
		else
		{
			InterlockedExchange8(&_session.bSending, false);
		}
	}
}
void IOCPClient::SendPost()
{
	WSABUF wsaBufs[MAX_SEND_BUF_CNT];
	_session.sendBufCnt = min(MAX_SEND_BUF_CNT, _session.sendBufQ.Size());
	for (int i = 0; i < _session.sendBufCnt; i++)
	{
		_session.sendBufQ.Dequeue(&(_session.pSendedBufArr[i]));
		if (_bWan)
		{
			wsaBufs[i].buf = (char*)(_session.pSendedBufArr[i])->GetWanHeader();
			wsaBufs[i].len = (_session.pSendedBufArr[i])->GetPayLoadSize() + sizeof(WanHeader);
		}
		else
		{
			wsaBufs[i].buf = (char*)(_session.pSendedBufArr[i])->GetLanHeader();
			wsaBufs[i].len = (_session.pSendedBufArr[i])->GetPayLoadSize() + sizeof(LanHeader);
		}
	}
	memset(&_session.sendOverLapped, 0, sizeof(_session.sendOverLapped));
	InterlockedIncrement16(&_session.sessionManageInfo.refCnt);
	int retval = WSASend(_session.socket, wsaBufs, _session.sendBufCnt, NULL, 0, &_session.sendOverLapped, NULL);
	if (retval == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		if (error != ERROR_IO_PENDING)
		{
			if (error != WSAECONNRESET && error != WSAECONNABORTED
				&& error != WSAENOTSOCK && error != WSAESHUTDOWN)
			{
				Log::LogOnFile(Log::SYSTEM_LEVEL, "WSASend() error: %d\n", error);
			}
			if (InterlockedDecrement16(&_session.sessionManageInfo.refCnt) == 0)
			{
				ReleaseSession();
			}
		}
	}
}
void IOCPClient::RequestSend()
{
	InterlockedIncrement16(&_session.sessionManageInfo.refCnt);
	bool retPQCS = PostQueuedCompletionStatus(_hcp, REQUEST_SEND, (ULONG_PTR)REQUEST_SEND, (LPOVERLAPPED)REQUEST_SEND);
	if (retPQCS == false)
	{
		Log::LogOnFile(Log::SYSTEM_LEVEL, "RequestSend error: %d\n", WSAGetLastError());
	}
}
void IOCPClient::Unicast(CSendBuffer* pBuf, bool bDisconnectAfterSend)
{
	if (IsSessionAvailable() == false)
	{
		return;
	}

	if (_session.bReservedDisconnect == true)
	{
		if (InterlockedDecrement16(&_session.sessionManageInfo.refCnt) == 0)
		{
			ReleaseSession();
		}
		return;
	}


	if (_session.sendBufQ.Size() < SENDQ_MAX_LEN)
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
		_session.sendBufQ.Enqueue(pBuf);
	}
	else
	{
		InterlockedExchange8(&_session.onConnecting, false);
		CancelIo((HANDLE)_session.socket);
	}

	if (bDisconnectAfterSend)
	{
		InterlockedExchange8(&_session.bReservedDisconnect, true);
	}

	if (GetSendAuthority() == true)
	{
		RequestSend();
	}

	if (InterlockedDecrement16(&_session.sessionManageInfo.refCnt) == 0)
	{
		ReleaseSession();
	}
	return;
}
bool IOCPClient::Connect()
{
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

	InitializeSession(serverSock);
	OnConnect();
	RecvPost();
	return true;
}

void IOCPClient::Disconnect()
{
	if (IsSessionAvailable()==false)
	{
		return;
	}
	InterlockedExchange8(&_session.onConnecting, false);
	CancelIoEx((HANDLE)_session.socket, NULL);
	if (InterlockedDecrement16(&_session.sessionManageInfo.refCnt) == 0)
	{
		ReleaseSession();
	}
}

unsigned __stdcall IOCPClient::IOCPWorkThreadFunc(LPVOID arg)
{
	IOCPClient* pClient = (IOCPClient*)arg;
	pClient->IOCPWork();
	return 0;
}

void IOCPClient::CreateThread(_beginthreadex_proc_type pFunction)
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
void IOCPClient::RecvCompletionRoutine()
{
	LONG recvCnt = 0;
	while (1)
	{
		int useSize = _session.recvBuffer.GetUseSize();

		if (useSize < sizeof(NetHeader))
		{
			break;
		}
		NetHeader netHeader;
		int peekSize = _session.recvBuffer.Peek((char*)&netHeader, sizeof(netHeader));
		if (peekSize != sizeof(netHeader))
		{
			Log::LogOnFile(Log::SYSTEM_LEVEL, "recvBuffer Peek Size error 종료\n");
			DebugBreak();
		}
		if constexpr (std::is_same<NetHeader, WanHeader>::value)
		{
			if (netHeader.code != WanHeader::NetCode)
			{
				InterlockedExchange8(&_session.onConnecting, false);
				break;
			}
		}
		if (netHeader.len > PAYLOAD_MAX_LEN)
		{
			InterlockedExchange8(&_session.onConnecting, false);
			break;
		}
		if (useSize < sizeof(NetHeader) + netHeader.len)
		{
			break;
		}
		_session.recvBuffer.MoveFront(sizeof(netHeader));
		CRecvBuffer buf(&_session.recvBuffer, netHeader.len);
		if constexpr (std::is_same<NetHeader, WanHeader>::value)
		{
			if (buf.Decode(&netHeader) == false)
			{
				InterlockedExchange8(&_session.onConnecting, false);
				break;
			}
		}
		OnRecv(buf);
		recvCnt++;
	}
	InterlockedAdd(&_recvCnt, recvCnt);
	RecvPost();
}

void IOCPClient::SendCompletionRoutine()
{
	InterlockedAdd(&_sendCnt, _session.sendBufCnt);
	for (int i = 0; i < _session.sendBufCnt; i++)
	{
		//OnSend(pSession->sessionInfo, pSession->pSendedBufArr[i]->GetPayLoadSize());
		(_session.pSendedBufArr[i])->DecrementRefCnt();
	}
	_session.sendBufCnt = 0;
	InterlockedExchange8(&_session.bSending, false);
	if (GetSendAuthority() == true)
	{
		SendPost();
	}
	else
	{
		if (_session.sendBufQ.Size() == 0 && _session.bReservedDisconnect==true)
		{
			Disconnect();
		}
	}
	if (InterlockedDecrement16(&_session.sessionManageInfo.refCnt) == 0)
	{
		ReleaseSession();
	}
}
void IOCPClient::RequestSendCompletionRoutine()
{
	SendPost();
	if (InterlockedDecrement16(&_session.sessionManageInfo.refCnt) == 0)
	{
		ReleaseSession();
	}
}
void IOCPClient::IOCPWork()
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
			Log::LogOnFile(Log::SYSTEM_LEVEL, "GQCS pOverlapped null error: %d\n", WSAGetLastError());
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
					ReleaseSession();
				}
				continue;
			}

			if (pOverlapped == &pSession->recvOverLapped)
			{
				pSession->recvBuffer.MoveBack(cbTransferred);
				if (_bWan)
				{
					RecvCompletionRoutine<WanHeader>();
				}
				else
				{
					RecvCompletionRoutine<LanHeader>();
				}
			}
			else if (pOverlapped == &pSession->sendOverLapped)
			{
				SendCompletionRoutine();
			}
			else if (pOverlapped == (LPOVERLAPPED)REQUEST_SEND)
			{
				RequestSendCompletionRoutine();
			}
			else if (pOverlapped == (LPOVERLAPPED)CLIENT_DOWN)
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

void IOCPClient::IOCPRun()
{
	for (int i = 0; i < IOCP_THREAD_NUM; i++)
	{
		CreateThread(IOCPClient::IOCPWorkThreadFunc);
	}
	return;
}
int IOCPClient::GetRecvCnt()
{
	int ret = _recvCnt;
	InterlockedExchange(&_recvCnt, 0);
	return ret;
}
int IOCPClient::GetSendCnt()
{
	int ret = _sendCnt;
	InterlockedExchange(&_sendCnt, 0);
	return ret;
}
void IOCPClient::SetMaxPayloadLen(int len)
{
	PAYLOAD_MAX_LEN = len;
	return;
}




