#pragma once
#include "MyWindow.h"
#include "CRecvBuffer.h"
#include "CSendBuffer.h"
#include "LockFreeStack.h"
#include "Session.h"
#include "MyStlContainer.h"
#include<process.h>
#include "LockQueue.h"
#include <type_traits>
#include <string>
class IOCPClient
{
private:
	void DropIoPending();
	HANDLE CreateNewCompletionPort(DWORD dwNumberOfConcurrentThreads);
	BOOL AssociateDeviceWithCompletionPort(HANDLE hCompletionPort, HANDLE hDevice, ULONG_PTR dwCompletionKey);

	void RecvPost();
	bool GetSendAuthority();
	void SendPost();
	void RequestSend();

	template<typename NetHeader>
	void RecvCompletionRoutine();
	void SendCompletionRoutine();
	void RequestSendCompletionRoutine();

	void IOCPWork();
	static unsigned __stdcall IOCPWorkThreadFunc(LPVOID arg);
	void CreateThread(_beginthreadex_proc_type pFunction);

	bool IsSessionAvailable();
	void InitializeSession(SOCKET clientSock);
	void ReleaseSession();
private:
	const long long EXIT_TIMEOUT = 5000;
	const long long SENDQ_MAX_LEN = 512;
	enum IOCP_KEY
	{
		CLIENT_DOWN = 100,
		REQUEST_SEND
	};
protected:
	std::string _settingFileName;
	int IOCP_THREAD_NUM = 0;
	int CONCURRENT_THREAD_NUM = 0;
	int PACKET_CODE = 0;
	int PACKET_KEY = 0;
	int LOG_LEVEL = 0;
	int PAYLOAD_MAX_LEN = 300;
	std::string SERVER_IP;
	int SERVER_PORT=0;
private:
	bool _bWan;
	HANDLE _hcp = INVALID_HANDLE_VALUE;
	List<HANDLE> _hThreadList;
	Session _session;
private:
	LONG _acceptCnt = 0;
	LONG _sendCnt = 0;
	LONG _recvCnt = 0;
public:
	IOCPClient(std::string settingFileName, bool bWan=true) : _bWan(bWan), _settingFileName(settingFileName)
	{
		GetClientSetValues(settingFileName);
		ClientSetting();
	}
	IOCPClient(bool bWan=true) : _bWan(bWan){}
	virtual ~IOCPClient()
	{
	}
	void Unicast(CSendBuffer* buf, bool bDisconnect = false);
	void Disconnect();
	void CloseClient();
protected:
	void IOCPRun();
	virtual void OnDisconnect() = 0;
	virtual void OnRecv(CRecvBuffer& buf) = 0;
	virtual void Run() = 0;
	void GetClientSetValues(std::string settingFileName);
	void ClientSetting();
public:
	bool Connect();
	int GetRecvCnt();
	int GetSendCnt();
	void	SetMaxPayloadLen(int len);
};

