#include  "testProxy.h"
#include "testPKT_TYPE.h"
void testProxy::CS_CHAT_REQ_LOGIN(SessionInfo sessionInfo, INT64& accountNo, WCHAR& id[20], WCHAR& nickName[20], char& sessionKey[64] )
{
	CSerialBuffer* pBuf = CSerialBuffer::Alloc();
	pBuf->IncrementRefCnt();
	try
	{
		*pBuf << accountNo << id[20] << nickName[20] << sessionKey[64];
	}
	catch(int useSize)
	{
	}
	_pServer->Unicast(sessionInfo, pBuf);
	pBuf->DecrementRefCnt();
}
void testProxy::CS_CHAT_REQ_LOGINPost(SessionInfo sessionInfo, INT64& accountNo, WCHAR& id[20], WCHAR& nickName[20], char& sessionKey[64] )
{
	CSerialBuffer* pBuf = CSerialBuffer::Alloc();
	pBuf->IncrementRefCnt();
	try
	{
		*pBuf << accountNo << id[20] << nickName[20] << sessionKey[64];
	}
	catch(int useSize)
	{
	}
	_pServer->UnicastPost(sessionInfo, pBuf);
	pBuf->DecrementRefCnt();
}
void testProxy::CS_CHAT_REQ_LOGIN(list<SessionInfo>& sessionInfoList, INT64& accountNo, WCHAR& id[20], WCHAR& nickName[20], char& sessionKey[64] )
{
	CSerialBuffer* pBuf = CSerialBuffer::Alloc();
	pBuf->IncrementRefCnt();
	try
	{
		*pBuf << accountNo << id[20] << nickName[20] << sessionKey[64];
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
void testProxy::CS_CHAT_REQ_LOGINPost(list<SessionInfo>& sessionInfoList, INT64& accountNo, WCHAR& id[20], WCHAR& nickName[20], char& sessionKey[64] )
{
	CSerialBuffer* pBuf = CSerialBuffer::Alloc();
	pBuf->IncrementRefCnt();
	try
	{
		*pBuf << accountNo << id[20] << nickName[20] << sessionKey[64];
	}
	catch(int useSize)
	{
	}
	for(SessionInfo sessionInfo: sessionInfoList)
	{
		_pServer->UnicastPost(sessionInfo, pBuf);
	}
	pBuf->DecrementRefCnt();
}
void testProxy::CS_CHAT_RES_LOGIN(SessionInfo sessionInfo, WORD& type, BYTE& status, INT64& accountNo )
{
	CSerialBuffer* pBuf = CSerialBuffer::Alloc();
	pBuf->IncrementRefCnt();
	try
	{
		*pBuf << type << status << accountNo;
	}
	catch(int useSize)
	{
	}
	_pServer->Unicast(sessionInfo, pBuf);
	pBuf->DecrementRefCnt();
}
void testProxy::CS_CHAT_RES_LOGINPost(SessionInfo sessionInfo, WORD& type, BYTE& status, INT64& accountNo )
{
	CSerialBuffer* pBuf = CSerialBuffer::Alloc();
	pBuf->IncrementRefCnt();
	try
	{
		*pBuf << type << status << accountNo;
	}
	catch(int useSize)
	{
	}
	_pServer->UnicastPost(sessionInfo, pBuf);
	pBuf->DecrementRefCnt();
}
void testProxy::CS_CHAT_RES_LOGIN(list<SessionInfo>& sessionInfoList, WORD& type, BYTE& status, INT64& accountNo )
{
	CSerialBuffer* pBuf = CSerialBuffer::Alloc();
	pBuf->IncrementRefCnt();
	try
	{
		*pBuf << type << status << accountNo;
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
void testProxy::CS_CHAT_RES_LOGINPost(list<SessionInfo>& sessionInfoList, WORD& type, BYTE& status, INT64& accountNo )
{
	CSerialBuffer* pBuf = CSerialBuffer::Alloc();
	pBuf->IncrementRefCnt();
	try
	{
		*pBuf << type << status << accountNo;
	}
	catch(int useSize)
	{
	}
	for(SessionInfo sessionInfo: sessionInfoList)
	{
		_pServer->UnicastPost(sessionInfo, pBuf);
	}
	pBuf->DecrementRefCnt();
}
void testProxy::CS_CHAT_REQ_SECTOR_MOVE(SessionInfo sessionInfo, WORD& type, INT64& accountNo, WORD& sectorX, WORD& sectorY )
{
	CSerialBuffer* pBuf = CSerialBuffer::Alloc();
	pBuf->IncrementRefCnt();
	try
	{
		*pBuf << type << accountNo << sectorX << sectorY;
	}
	catch(int useSize)
	{
	}
	_pServer->Unicast(sessionInfo, pBuf);
	pBuf->DecrementRefCnt();
}
void testProxy::CS_CHAT_REQ_SECTOR_MOVEPost(SessionInfo sessionInfo, WORD& type, INT64& accountNo, WORD& sectorX, WORD& sectorY )
{
	CSerialBuffer* pBuf = CSerialBuffer::Alloc();
	pBuf->IncrementRefCnt();
	try
	{
		*pBuf << type << accountNo << sectorX << sectorY;
	}
	catch(int useSize)
	{
	}
	_pServer->UnicastPost(sessionInfo, pBuf);
	pBuf->DecrementRefCnt();
}
void testProxy::CS_CHAT_REQ_SECTOR_MOVE(list<SessionInfo>& sessionInfoList, WORD& type, INT64& accountNo, WORD& sectorX, WORD& sectorY )
{
	CSerialBuffer* pBuf = CSerialBuffer::Alloc();
	pBuf->IncrementRefCnt();
	try
	{
		*pBuf << type << accountNo << sectorX << sectorY;
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
void testProxy::CS_CHAT_REQ_SECTOR_MOVEPost(list<SessionInfo>& sessionInfoList, WORD& type, INT64& accountNo, WORD& sectorX, WORD& sectorY )
{
	CSerialBuffer* pBuf = CSerialBuffer::Alloc();
	pBuf->IncrementRefCnt();
	try
	{
		*pBuf << type << accountNo << sectorX << sectorY;
	}
	catch(int useSize)
	{
	}
	for(SessionInfo sessionInfo: sessionInfoList)
	{
		_pServer->UnicastPost(sessionInfo, pBuf);
	}
	pBuf->DecrementRefCnt();
}
void testProxy::CS_CHAT_RES_SECTOR_MOVE(SessionInfo sessionInfo, WORD& type, INT64& accountNo, WORD& sectorX, WORD& sectorY )
{
	CSerialBuffer* pBuf = CSerialBuffer::Alloc();
	pBuf->IncrementRefCnt();
	try
	{
		*pBuf << type << accountNo << sectorX << sectorY;
	}
	catch(int useSize)
	{
	}
	_pServer->Unicast(sessionInfo, pBuf);
	pBuf->DecrementRefCnt();
}
void testProxy::CS_CHAT_RES_SECTOR_MOVEPost(SessionInfo sessionInfo, WORD& type, INT64& accountNo, WORD& sectorX, WORD& sectorY )
{
	CSerialBuffer* pBuf = CSerialBuffer::Alloc();
	pBuf->IncrementRefCnt();
	try
	{
		*pBuf << type << accountNo << sectorX << sectorY;
	}
	catch(int useSize)
	{
	}
	_pServer->UnicastPost(sessionInfo, pBuf);
	pBuf->DecrementRefCnt();
}
void testProxy::CS_CHAT_RES_SECTOR_MOVE(list<SessionInfo>& sessionInfoList, WORD& type, INT64& accountNo, WORD& sectorX, WORD& sectorY )
{
	CSerialBuffer* pBuf = CSerialBuffer::Alloc();
	pBuf->IncrementRefCnt();
	try
	{
		*pBuf << type << accountNo << sectorX << sectorY;
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
void testProxy::CS_CHAT_RES_SECTOR_MOVEPost(list<SessionInfo>& sessionInfoList, WORD& type, INT64& accountNo, WORD& sectorX, WORD& sectorY )
{
	CSerialBuffer* pBuf = CSerialBuffer::Alloc();
	pBuf->IncrementRefCnt();
	try
	{
		*pBuf << type << accountNo << sectorX << sectorY;
	}
	catch(int useSize)
	{
	}
	for(SessionInfo sessionInfo: sessionInfoList)
	{
		_pServer->UnicastPost(sessionInfo, pBuf);
	}
	pBuf->DecrementRefCnt();
}
void testProxy::CS_CHAT_REQ_MESSAGE(SessionInfo sessionInfo, WORD& type, INT64& accountNo, WORD& messageLen, & WCHAR )
{
	CSerialBuffer* pBuf = CSerialBuffer::Alloc();
	pBuf->IncrementRefCnt();
	try
	{
		*pBuf << type << accountNo << messageLen << WCHAR;
	}
	catch(int useSize)
	{
	}
	_pServer->Unicast(sessionInfo, pBuf);
	pBuf->DecrementRefCnt();
}
void testProxy::CS_CHAT_REQ_MESSAGEPost(SessionInfo sessionInfo, WORD& type, INT64& accountNo, WORD& messageLen, & WCHAR )
{
	CSerialBuffer* pBuf = CSerialBuffer::Alloc();
	pBuf->IncrementRefCnt();
	try
	{
		*pBuf << type << accountNo << messageLen << WCHAR;
	}
	catch(int useSize)
	{
	}
	_pServer->UnicastPost(sessionInfo, pBuf);
	pBuf->DecrementRefCnt();
}
void testProxy::CS_CHAT_REQ_MESSAGE(list<SessionInfo>& sessionInfoList, WORD& type, INT64& accountNo, WORD& messageLen, & WCHAR )
{
	CSerialBuffer* pBuf = CSerialBuffer::Alloc();
	pBuf->IncrementRefCnt();
	try
	{
		*pBuf << type << accountNo << messageLen << WCHAR;
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
void testProxy::CS_CHAT_REQ_MESSAGEPost(list<SessionInfo>& sessionInfoList, WORD& type, INT64& accountNo, WORD& messageLen, & WCHAR )
{
	CSerialBuffer* pBuf = CSerialBuffer::Alloc();
	pBuf->IncrementRefCnt();
	try
	{
		*pBuf << type << accountNo << messageLen << WCHAR;
	}
	catch(int useSize)
	{
	}
	for(SessionInfo sessionInfo: sessionInfoList)
	{
		_pServer->UnicastPost(sessionInfo, pBuf);
	}
	pBuf->DecrementRefCnt();
}
void testProxy::CS_CHAT_RES_MESSAGE(SessionInfo sessionInfo, WORD& type, INT64& accountNo, WCHAR& id[20], WCHAR& nickName[20], WORD& messageLen, WString& message )
{
	CSerialBuffer* pBuf = CSerialBuffer::Alloc();
	pBuf->IncrementRefCnt();
	try
	{
		*pBuf << type << accountNo << id[20] << nickName[20] << messageLen << message;
	}
	catch(int useSize)
	{
	}
	_pServer->Unicast(sessionInfo, pBuf);
	pBuf->DecrementRefCnt();
}
void testProxy::CS_CHAT_RES_MESSAGEPost(SessionInfo sessionInfo, WORD& type, INT64& accountNo, WCHAR& id[20], WCHAR& nickName[20], WORD& messageLen, WString& message )
{
	CSerialBuffer* pBuf = CSerialBuffer::Alloc();
	pBuf->IncrementRefCnt();
	try
	{
		*pBuf << type << accountNo << id[20] << nickName[20] << messageLen << message;
	}
	catch(int useSize)
	{
	}
	_pServer->UnicastPost(sessionInfo, pBuf);
	pBuf->DecrementRefCnt();
}
void testProxy::CS_CHAT_RES_MESSAGE(list<SessionInfo>& sessionInfoList, WORD& type, INT64& accountNo, WCHAR& id[20], WCHAR& nickName[20], WORD& messageLen, WString& message )
{
	CSerialBuffer* pBuf = CSerialBuffer::Alloc();
	pBuf->IncrementRefCnt();
	try
	{
		*pBuf << type << accountNo << id[20] << nickName[20] << messageLen << message;
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
void testProxy::CS_CHAT_RES_MESSAGEPost(list<SessionInfo>& sessionInfoList, WORD& type, INT64& accountNo, WCHAR& id[20], WCHAR& nickName[20], WORD& messageLen, WString& message )
{
	CSerialBuffer* pBuf = CSerialBuffer::Alloc();
	pBuf->IncrementRefCnt();
	try
	{
		*pBuf << type << accountNo << id[20] << nickName[20] << messageLen << message;
	}
	catch(int useSize)
	{
	}
	for(SessionInfo sessionInfo: sessionInfoList)
	{
		_pServer->UnicastPost(sessionInfo, pBuf);
	}
	pBuf->DecrementRefCnt();
}
void testProxy::CS_CHAT_REQ_HEARTBEAT(SessionInfo sessionInfo, WORD& type )
{
	CSerialBuffer* pBuf = CSerialBuffer::Alloc();
	pBuf->IncrementRefCnt();
	try
	{
		*pBuf << type;
	}
	catch(int useSize)
	{
	}
	_pServer->Unicast(sessionInfo, pBuf);
	pBuf->DecrementRefCnt();
}
void testProxy::CS_CHAT_REQ_HEARTBEATPost(SessionInfo sessionInfo, WORD& type )
{
	CSerialBuffer* pBuf = CSerialBuffer::Alloc();
	pBuf->IncrementRefCnt();
	try
	{
		*pBuf << type;
	}
	catch(int useSize)
	{
	}
	_pServer->UnicastPost(sessionInfo, pBuf);
	pBuf->DecrementRefCnt();
}
void testProxy::CS_CHAT_REQ_HEARTBEAT(list<SessionInfo>& sessionInfoList, WORD& type )
{
	CSerialBuffer* pBuf = CSerialBuffer::Alloc();
	pBuf->IncrementRefCnt();
	try
	{
		*pBuf << type;
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
void testProxy::CS_CHAT_REQ_HEARTBEATPost(list<SessionInfo>& sessionInfoList, WORD& type )
{
	CSerialBuffer* pBuf = CSerialBuffer::Alloc();
	pBuf->IncrementRefCnt();
	try
	{
		*pBuf << type;
	}
	catch(int useSize)
	{
	}
	for(SessionInfo sessionInfo: sessionInfoList)
	{
		_pServer->UnicastPost(sessionInfo, pBuf);
	}
	pBuf->DecrementRefCnt();
}
