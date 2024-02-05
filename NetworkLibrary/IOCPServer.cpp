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
//#include "Log.h"
using namespace rapidjson;
using namespace std;
const long long EXIT_TIMEOUT = 5000;
const long long SERVER_DOWN_KEY = 200;
const long long REQUEST_SEND = 300;


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

void ShowCurrentTime()
{
	__time64_t now = time(0);
	tm* timeinfo = _localtime64(&now);
	cout << "���� �ð�: ";
	cout << (timeinfo->tm_year + 1900) << '-'
		<< (timeinfo->tm_mon + 1) << '-'
		<< timeinfo->tm_mday << ' '
		<< timeinfo->tm_hour << ':'
		<< timeinfo->tm_min << ':'
		<< timeinfo->tm_sec << '\n';
}

void IOCPServer::GetSeverSetValues()
{
	ifstream fin(_settingFileName);
	if (!fin)
	{
		Log::Printf(Log::SYSTEM_LEVEL, _settingFileName.data());
		Sleep(10000);
		DebugBreak();
	}
	string json((istreambuf_iterator<char>(fin)), (istreambuf_iterator<char>()));
	fin.close();
	Document d;
	rapidjson::ParseResult parseResult = d.Parse(json.data());
	if (!parseResult) {
		fprintf(stderr, "JSON parse error: %s (%u)",GetParseError_En(parseResult.Code()), parseResult.Offset());
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

	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(BIND_PORT);
	ret_bind = bind(_listenSock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (ret_bind == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		cout << "bind() error : " << error << '\n';
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

	_sessionArray = new Session[SESSION_MAX];
	for (int i = 0; i < SESSION_MAX; i++)
	{
		_validIndexStack.Push(i);
	}

	NetHeader::SetConstKey(PACKET_KEY);
	NetHeader::SetNetCode(PACKET_CODE);
	Log::SetLogLevel(LOG_LEVEL);

	_hcp = CreateNewCompletionPort(CONCURRENT_THREAD_NUM);
	if (_hcp == NULL)
	{
		int error = WSAGetLastError();
		cout << "CreateNewCompletionPort error : " << error << endl;
		DebugBreak();
	};
	for (int i = 0; i < IOCP_THREAD_NUM; i++)
	{
		RegisterThread(IOCPServer::IOCPWorkThreadFunc);
	}
}

Session* IOCPServer::FindSession(SessionInfo sessionInfo)
{
	Session* pSession = &_sessionArray[sessionInfo.index.val];
	InterlockedIncrement16(&pSession->sessionManageInfo.refCnt);
	if (pSession->sessionManageInfo.bDeallocated == true)
	{
		//Release�� �̹� �ǰ� �ִ�. �̵̹ǰ� �Ҵ� �޴����ε� Release�� �Ǵ� ���� <-���� a -> �޴����̶� bDeallocated==false�� �� ��� release �� �� ����.
		if (InterlockedDecrement16(&pSession->sessionManageInfo.refCnt) == 0)
		{
			ReleaseSession(pSession);
		}
		return nullptr;
	}

	//Release�� �ȵǰ� �ִ�->bDeallocated==false case 1. �׳� ��� �����ΰ�� , case 2 Release�� �ǰ� �� �̹� �ٸ�id�� ���ȴ� <-���� b.
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
		DebugBreak();
	}

	Session* pSession = &_sessionArray[index];
	//���⼭ ���� a �߻� �ϴ°� bDeallocated == true�λ���->�������

	InterlockedIncrement16(&pSession->sessionManageInfo.refCnt);

	//���� b�� ���� ���� id�� �Ҵ��ϰ� bDeallocated ������ �ٲ۴�.

	pSession->sessionInfo.id = _newSessionID++;
	pSession->sessionInfo.index.val = index;
	//������� bDeallocated == true�̴�
	pSession->sessionManageInfo.bDeallocated = false;
	return pSession;
}

void IOCPServer::DisConnect(Session* pSession)
{
	SOCKET socket = pSession->socket;
	pSession->socket = INVALID_SOCKET;
	closesocket(socket);
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
		// refCnt ==0 && bDeallocated == false �� �ƴ� ���
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
	pSession->recvBuffer.ClearBuffer();
	_validIndexStack.Push(pSession->sessionInfo.index.val);
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
		//Log��
		_acceptCnt++;
		
		if (clientSock == INVALID_SOCKET)
		{
			int error = WSAGetLastError();
			if (error != WSAEINTR)
			{
				Log::Printf(Log::SYSTEM_LEVEL, "accept error : %d\n", error);
			}
			break;
		}
		if (_validIndexStack.Size()<=0)
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
			Log::Printf(Log::SYSTEM_LEVEL, "AssociateDeviceWithCompletionPort error : %d\n", WSAGetLastError());
			DebugBreak();
		}

		pSession->socket = clientSock;
		pSession->bSending = false;
		strcpy_s(pSession->ip, INET_ADDRSTRLEN, ip);
		pSession->port = port;
		OnConnect(pSession->sessionInfo);

		RecvPost(pSession);
	}
	return;
}

void IOCPServer::CloseServer()
{
	for (int i = 0; i <SESSION_MAX; i++)
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
			Log::Printf(Log::SYSTEM_LEVEL, "%d ������ ����\n", GetThreadId(hThread));
		}
		else
		{
			DebugBreak();
		}
		CloseHandle(hThread);
	}
	CloseHandle(_hcp);
	WSACleanup();
	delete[] _sessionArray;
}

void IOCPServer::RecvPost(Session* pSession)
{
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
				&& error != WSAENOTSOCK && error != WSAESHUTDOWN)
			{
				Log::Printf(Log::SYSTEM_LEVEL,"WSARecv() error: %d\n", error);
			}
			//DisConnect(pSession);
			if (InterlockedDecrement16(&pSession->sessionManageInfo.refCnt) == 0)
			{
				ReleaseSession(pSession);
			}
		}
	}

}

bool IOCPServer::GetSendAuthority(Session* pSession)
{
	while (1)
	{
		if (pSession->sendBufQ.Size() == 0 || InterlockedExchange(&pSession->bSending, true) != false)
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
	for (int i = 0; i < MAX_SEND_BUF_CNT; i++)
	{
		if (pSession->sendBufQ.Size()>0)
		{
			pSession->sendBufQ.Dequeue(&(pSession->pSendedBufArr[i]));
			wsaBufs[i].buf = (pSession->pSendedBufArr[i])->_buf;
			wsaBufs[i].len = (pSession->pSendedBufArr[i])->GetPacketSize();
			pSession->sendBufCnt++;
		}
		else
		{
			break;
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
				Log::Printf(Log::SYSTEM_LEVEL, "WSASend() error: %d\n", error);
			}
			//DisConnect(pSession);
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
		Log::Printf(Log::SYSTEM_LEVEL, "RequestSend error: %d\n", WSAGetLastError());
	}
}

void IOCPServer::Unicast(SessionInfo sessionInfo, CSendBuffer* pBuf)
{
	Session* pSession = FindSession(sessionInfo);
	if (pSession == nullptr)
	{
		return;
	}

	if (pSession->sendBufQ.Size() < SENDQ_MAX_LEN)
	{
		pBuf->IncrementRefCnt();
		pSession->sendBufQ.Enqueue(pBuf);
	}
	else
	{
		pBuf->DecrementRefCnt();
		DisConnect(pSession);
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

void IOCPServer::DisConnect(SessionInfo sessionInfo)
{
	Session* pSession = FindSession(sessionInfo);
	if (pSession == nullptr)
	{
		return ;
	}
	SOCKET socket=pSession->socket;
	pSession->socket = INVALID_SOCKET;
	closesocket(socket);
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

void IOCPServer::RegisterThread(_beginthreadex_proc_type pFunction)
{
	HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, pFunction, this, 0, NULL);
	if (hThread == NULL)
	{
		Log::Printf(Log::SYSTEM_LEVEL, "_beginthreadex error: %d\n", WSAGetLastError());
		DebugBreak();
	}
	_hThreadList.push_back(hThread);
}

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
			DebugBreak();
		}

		if (useSize < sizeof(NetHeader) + netHeader.len)
		{
			break;
		}
		pSession->recvBuffer.MoveFront(sizeof(netHeader));
		CRecvBuffer buf(&pSession->recvBuffer, netHeader.len);
		if (_bWan)
		{
			buf.Decode(&netHeader);
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
	if (GetSendAuthority(pSession) == true)
	{
		SendPost(pSession);
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

		OVERLAPPED* pOverlapped = nullptr;
		int retval = GetQueuedCompletionStatus(_hcp, &cbTransferred, (PULONG_PTR)&pSession, &pOverlapped, INFINITE);
		if (pOverlapped == nullptr)
		{
			if ((ULONG_PTR)pSession != SERVER_DOWN_KEY)
			{
				Log::Printf(Log::SYSTEM_LEVEL, "gqcs error: %d\n", WSAGetLastError());
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
						ShowCurrentTime();
						Log::Printf(Log::SYSTEM_LEVEL, "gqcs ret false error: %d\n", error);
					}
				}
				//DisConnect(pSession);
				if (InterlockedDecrement16(&pSession->sessionManageInfo.refCnt) == 0)
				{
					ReleaseSession(pSession);
				}
				continue;
			}

			if (pOverlapped  == &pSession->recvOverLapped )
			{
				pSession->recvBuffer.MoveBack(cbTransferred);
				RecvCompletionRoutine(pSession);
			}
			else if (pOverlapped  == &pSession->sendOverLapped )
			{
				SendCompletionRoutine(pSession);
			}
			else if (pOverlapped == (OVERLAPPED*)REQUEST_SEND)
			{
				RequestSendCompletionRoutine(pSession);
			}
			else
			{
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
		cout << "listen() error : " << error << endl;
		DebugBreak();
	}
	RegisterThread(IOCPServer::AcceptThreadFunc);
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
