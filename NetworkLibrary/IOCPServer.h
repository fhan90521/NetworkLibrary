#pragma once
#pragma comment(lib,"ws2_32.lib")
#pragma comment (lib,"Winmm.lib")
#include "MyWindow.h"
#include "CMirrorBuffer.h"
#include "CSerialBuffer.h"
#include "Session.h"
#include<list>
#include<vector>
#include<stack>
using namespace std;
class IOCPServer
{
private:

	//void ServerInitial();
	void GetSeverSetValues();

	void CloseServer();
	
	void RecvPost(Session* pSession);
	void SendPost(Session* pSession);
	void RecvCompletionRoutine(Session* pSession);
	void SendCompletionRoutine(Session* pSession);

	void AcceptWork();
	static unsigned __stdcall AcceptThreadFunc(LPVOID arg);
	void IOCPWork();
	static unsigned __stdcall IOCPWorkThreadFunc(LPVOID arg);

	Session* FindSession(SessionInfo sessionInfo);
	Session* AllocSession(SOCKET clientSock);
	void ReleaseSession(Session* pSession);
private:
	
	int IOCP_THREAD_NUM = 0;
	int CONCURRENT_THREAD_NUM = 0;
	int SERVER_PORT = 0;
	int MAX_SEND_BUF_CNT = 0;
private:
	SOCKET _listenSock=INVALID_SOCKET;
	DWORD _newSessionID = 0;
	HANDLE _hcp=INVALID_HANDLE_VALUE;
	list<HANDLE> _hThreadList;
	Session* _sessionArray;
	stack<USHORT,vector<USHORT>> _validIndexStack;
	SRWLOCK _stackLock;
	unsigned short _arraySize;
	const bool _bWan;
public:
	IOCPServer(USHORT arraySize,bool bWan): _arraySize(arraySize),_bWan(bWan)
	{
		InitializeSRWLock(&_stackLock);
		_sessionArray = new Session[arraySize];
		for (int i = 0; i < arraySize; i++)
		{
			_validIndexStack.push(i);
		}
		_arraySize = arraySize;
		GetSeverSetValues();
	}
	~IOCPServer()
	{
		CloseServer();
		delete[] _sessionArray;
	}
	void IOCPRun();
	void ServerControl();
	void Unicast(SessionInfo sessionInfo, CSerialBuffer* buf);
	void DisConnect(SessionInfo sessionInfo);

	bool _bShutdown=false;
	virtual bool OnConnectRequest(const char* ip,USHORT port)=0;
	virtual void OnConnect(SessionInfo sessionInfo)=0;
	virtual void OnDisConnect(SessionInfo sessionInfo)=0;
	virtual void OnSend(SessionInfo sessionInfo)=0;
	virtual void OnRecv(SessionInfo sessionInfo, CMirrorBuffer& buf)=0;
	//virtual void OnWorkerThreadBegin() = 0; 
	//virtual void OnWorkerThreadEnd() = 0;          
	//virtual void OnError(int errorcode, char* log) = 0;

	virtual void Run()=0;
	int _acceptCnt=0;
	LONG _sendCnt=0;
	LONG _recvCnt=0;
public:
	int GetAcceptTPS();
	int GetRecvTPS();
	int GetSendTPS();
};

