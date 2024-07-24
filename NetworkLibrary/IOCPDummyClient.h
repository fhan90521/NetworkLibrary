#pragma once
#include "MyWindow.h"
#include "CRecvBuffer.h"
#include "CSendBuffer.h"
#include "LockFreeStack.h"
#include "Session.h"
#include "MyStlContainer.h"
#include<process.h>
#include <type_traits>
#include "LockQueue.h"
class IOCPDummyClient
{
private:
	enum IOCP_KEY
	{
		DUMMY_DOWN = 100,
		REQUEST_SEND,
	};
private:
	void DropIoPending(SessionInfo sessionInfo);
	void GetDummySetValues(std::string settingFileName);
	void DummySetting();
	HANDLE CreateNewCompletionPort(DWORD dwNumberOfConcurrentThreads);
	BOOL AssociateDeviceWithCompletionPort(HANDLE hCompletionPort, HANDLE hDevice, ULONG_PTR dwCompletionKey);

	void RecvPost(Session* pSession);

	bool GetSendAuthority(Session* pSession);
	void SendPost(Session* pSession);
	void RequestSend(Session* pSession);
	
	template<typename NetHeader>
	void RecvCompletionRoutine(Session* pSession);
	
	void SendCompletionRoutine(Session* pSession);
	void RequestSendCompletionRoutine(Session* pSession);

	void IOCPWork();
	static unsigned __stdcall IOCPWorkThreadFunc(LPVOID arg);
	void CreateThread(_beginthreadex_proc_type pFunction);
	
	Session* FindSession(SessionInfo sessionInfo);
	Session* AllocSession(SOCKET clientSock);
	void ReleaseSession(Session* pSession);
private:
	const long long EXIT_TIMEOUT = 5000;
	const long long SENDQ_MAX_LEN = 512;
protected:
	int IOCP_THREAD_NUM = 0;
	int CONCURRENT_THREAD_NUM = 0;
	int SERVER_PORT = 0;
	int SESSION_MAX = 0;
	int PACKET_CODE = 0;
	int PACKET_KEY = 0;
	int LOG_LEVEL = 0;
	int PAYLOAD_MAX_LEN = 300;
	bool _bWan;
	std::string SERVER_IP;
private:
	DWORD _newSessionID = 0;
	HANDLE _hcp=INVALID_HANDLE_VALUE;
	List<HANDLE> _hThreadList;
	Session* _sessionArray;
	LockFreeStack<USHORT> _validIndexStack;
private:
	LONG _acceptCnt = 0;
	LONG _sendCnt = 0;
	LONG _recvCnt = 0;
public:
	IOCPDummyClient(std::string settingFileName, bool bWan=true) : _bWan(bWan)
	{
		GetDummySetValues(settingFileName);
		DummySetting();
	}
	virtual ~IOCPDummyClient()
	{
	}
	
	CHAR _bShutdown = false;
	bool DummyControl();
	void Unicast(SessionInfo sessionInfo, CSendBuffer* buf, bool bDisconnect=false);
	void Disconnect(SessionInfo sessionInfo);
	void CloseDummy();
protected:
	void IOCPRun();
	virtual void OnConnect(SessionInfo sessionInfo) = 0;
	virtual void OnDisconnect(SessionInfo sessionInfo)=0;
	virtual void OnRecv(SessionInfo sessionInfo, CRecvBuffer& buf)=0;
	virtual void Run() = 0;
public:
	int GetAcceptCnt();
	int GetRecvCnt();
	int GetSendCnt();
	int GetConnectingSessionCnt();
	void	SetMaxPayloadLen(int len);

	//Connect는 멀티스레드에서의 호출을 지원하지 않음
	bool Connect();
//Disconnect After Send//
private:
	enum
	{
		RESERVE_DISCONNECT_MS=100
	};
	struct ReserveInfo
	{
		ULONG64 reserveTime;
		SessionInfo sessionInfo;
	};
	HANDLE _hShutDownEvent;
	HANDLE _hReserveDisconnectEvent;
	void ReserveDisconnectManage();
	static unsigned __stdcall ReserveDisconnectManageThreadFunc(LPVOID arg);
public:
	LockFreeQueue<ReserveInfo> _reserveDisconnectQ;
	List< ReserveInfo> _reserveDisconnectList;
};

