#include "Proxy.h"
#include "PKT_TYPE.h"
void Proxy::Echo(SessionInfo sessionInfo, long long& testLL )
{
	CSerialBuffer* pBuf = new CSerialBuffer;
	pBuf->IncrementRefCnt();
	try
	{
		*pBuf << testLL;
	}
	catch(int useSize)
	{
	}
	_pServer->Unicast(sessionInfo, pBuf);
	pBuf->DecrementRefCnt();
}
void Proxy::Echo(list<SessionInfo>& sessionInfoList, long long& testLL )
{
	CSerialBuffer* pBuf = new CSerialBuffer;
	pBuf->IncrementRefCnt();
	try
	{
		*pBuf << testLL;
	}
	catch(int useSize)
	{
	}
	for(SessionInfo sessionInfo: sessionInfoList)
	{
		_pServer->Unicast(sessionInfo, pBuf);
	}
	pBuf->DecrementRefCnt();
}
