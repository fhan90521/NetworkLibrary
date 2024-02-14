#pragma once
#pragma comment(lib,"ws2_32.lib")
#pragma comment (lib,"Winmm.lib")
#include "MyWindow.h"
#include "CRecvBuffer.h"
#include "CSendBuffer.h"
#include "LockFreeStack.h"
#include "Session.h"
#include<list>
#include<process.h>
#include<vector>
#include<stack>
using namespace std;
class IOCPServer
{
private:
	enum
	{
		SENDQ_MAX_LEN = 1000,
	};
	void DropIoPending(SessionInfo sessionInfo);
	void GetSeverSetValues();
	void ServerSetting();
	void CloseServer();
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
	
	void RegisterThread(_beginthreadex_proc_type pFunction);
	
	Session* FindSession(SessionInfo sessionInfo);
	Session* AllocSession(SOCKET clientSock);
	void ReleaseSession(Session* pSession);
private:
	string _settingFileName;
	int IOCP_THREAD_NUM = 0;
	int CONCURRENT_THREAD_NUM = 0;
	int BIND_PORT = 0;
	int SESSION_MAX = 0;
	int PACKET_CODE = 0;
	int PACKET_KEY = 0;
	int LOG_LEVEL = 0;
	int PAYLOAD_MAX_LEN = 300;
	bool _bWan;
protected:
	string BIND_IP;
private:
	SOCKET _listenSock=INVALID_SOCKET;
	DWORD _newSessionID = 0;
	HANDLE _hcp=INVALID_HANDLE_VALUE;
	list<HANDLE> _hThreadList;
	Session* _sessionArray;
	LockFreeStack<USHORT> _validIndexStack;
	LONG _acceptCnt = 0;
	LONG _sendCnt = 0;
	LONG _recvCnt = 0;
public:
	IOCPServer(bool bWan=true, string settingFileName = "ServerSetting.json") : _bWan(bWan), _settingFileName(settingFileName)
	{
		ServerSetting();
	}
	virtual ~IOCPServer()
	{
		CloseServer();
	}
	
	bool _bShutdown = false;
	void ServerControl();
	void Unicast(SessionInfo sessionInfo, CSendBuffer* buf);
	void Disconnect(SessionInfo sessionInfo);

protected:
	void IOCPRun();
	virtual bool OnConnectRequest(const char* ip,USHORT port)=0;
	virtual void OnConnect(SessionInfo sessionInfo)=0;
	virtual void OnDisconnect(SessionInfo sessionInfo)=0;
	//virtual void OnSend(SessionInfo sessionInfo, int sendSize)=0;
	virtual void OnRecv(SessionInfo sessionInfo, CRecvBuffer& buf)=0;
	virtual void Run() = 0;
	//virtual void OnWorkerThreadBegin() = 0; 
	//virtual void OnWorkerThreadEnd() = 0;          
	//virtual void OnError(int errorcode, char* log) = 0;
public:
	int GetAcceptCnt();
	int GetRecvCnt();
	int GetSendCnt();
	int GetConnectingSessionCnt();
	void SetMaxPayloadLen(int len);
};

