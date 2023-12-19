#pragma once
#include "Session.h"
#include "IOCPServer.h"
#include <list>
#include "PKT_TYPE.h"
using namespace std;
class Proxy
{
private:
	IOCPServer* _pServer;
public:
	void Echo(SessionInfo sessionInfo, long long& testLL );
	void EchoPost(SessionInfo sessionInfo, long long& testLL );
	void Echo(list<SessionInfo>& sessionInfoList, long long& testLL );
	void EchoPost(list<SessionInfo>& sessionInfoList, long long& testLL );
	Proxy(IOCPServer* pServer)
	{
		_pServer=pServer;
	}
};
