#pragma once
#include "MyWindow.h"
#include "CRecvBuffer.h"
#include "CSendBuffer.h"
#include "LockStack.h"
#include "Session.h"
#include "MyStlContainer.h"
#include <process.h>
#include <type_traits>
#include "MPSCQueue.h"
class IOCPServer
{
private:
	void DropIoPending(SessionInfo sessionInfo);
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
	
	void AcceptWork();
	static unsigned __stdcall AcceptThreadFunc(LPVOID arg);
	void IOCPWork();
	static unsigned __stdcall IOCPWorkThreadFunc(LPVOID arg);
	void CreateThread(_beginthreadex_proc_type pFunction);
	
	Session* FindSession(SessionInfo sessionInfo);
	Session* AllocSession(SOCKET clientSock);
	void ReleaseSession(Session* pSession);
	const long long EXIT_TIMEOUT = 5000;
private:
	int SENDQ_MAX_LEN = 1024;
	int IOCP_THREAD_NUM = 0;
	int CONCURRENT_THREAD_NUM = 0;
	int BIND_PORT = 0;
	int SESSION_MAX = 0;
	int PACKET_CODE = 0;
	int PACKET_KEY = 0;
	int LOG_LEVEL = 0;
	int PAYLOAD_MAX_LEN = 300;
	bool _bWan;
	std::string BIND_IP;
	void GetSeverSetValues(std::string settingFileName);
	void ServerSetting();
private:
	SOCKET _listenSock=INVALID_SOCKET;
	ULONG64 _newSessionID = 0;
	HANDLE _hcp=INVALID_HANDLE_VALUE;
	List<HANDLE> _hThreadList;
	Session* _sessionArray;
	LockStack<USHORT> _validIndexStack;
private:
	LONG _acceptCnt = 0;
	LONG _sendCnt = 0;
	LONG _recvCnt = 0;
public:
	IOCPServer(std::string settingFileName, bool bWan=true) : _bWan(bWan)
	{
		GetSeverSetValues(settingFileName);
		ServerSetting();
	}

	virtual ~IOCPServer(){}

	HANDLE GetCompletionPortHandle();
	CHAR _bShutdown = false;
	bool ServerControl();
	void Unicast(SessionInfo sessionInfo, CSendBuffer* buf, bool bDisconnect=false);
	void Disconnect(SessionInfo sessionInfo);
	bool GetClientIp(SessionInfo sessionInfo, String& outPar);
	void CloseServer();
protected:
	void IOCPRun();

private:

	virtual bool OnAcceptRequest(const char* ip,USHORT port)=0;
	virtual void OnAccept(SessionInfo sessionInfo)=0;
	virtual void OnDisconnect(SessionInfo sessionInfo)=0;
	virtual void OnRecv(SessionInfo sessionInfo, CRecvBuffer& buf)=0;

public:
	virtual void Run() = 0;
	int GetAcceptCnt();
	int GetRecvCnt();
	int GetSendCnt();
	int GetConnectingSessionCnt();
	void	 SetMaxPayloadLen(int len);

//Disconnect After Send//
private:
	enum: int
	{
		RESERVE_DISCONNECT_MS=3000
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
	MPSCQueue<ReserveInfo> _reserveDisconnectQ;
	List< ReserveInfo> _reserveDisconnectList;

};

