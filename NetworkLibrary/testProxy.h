#pragma once
#include "Session.h"
#include "IOCPServer.h"
#include <list>
using namespace std;
class testProxy
{
private:
	IOCPServer* _pServer;
public:
	void CS_CHAT_REQ_LOGIN(SessionInfo sessionInfo, INT64& accountNo, WCHAR& id[20], WCHAR& nickName[20], char& sessionKey[64] );
	void CS_CHAT_REQ_LOGINPost(SessionInfo sessionInfo, INT64& accountNo, WCHAR& id[20], WCHAR& nickName[20], char& sessionKey[64] );
	void CS_CHAT_REQ_LOGIN(list<SessionInfo>& sessionInfoList, INT64& accountNo, WCHAR& id[20], WCHAR& nickName[20], char& sessionKey[64] );
	void CS_CHAT_REQ_LOGINPost(list<SessionInfo>& sessionInfoList, INT64& accountNo, WCHAR& id[20], WCHAR& nickName[20], char& sessionKey[64] );
	void CS_CHAT_RES_LOGIN(SessionInfo sessionInfo, WORD& type, BYTE& status, INT64& accountNo );
	void CS_CHAT_RES_LOGINPost(SessionInfo sessionInfo, WORD& type, BYTE& status, INT64& accountNo );
	void CS_CHAT_RES_LOGIN(list<SessionInfo>& sessionInfoList, WORD& type, BYTE& status, INT64& accountNo );
	void CS_CHAT_RES_LOGINPost(list<SessionInfo>& sessionInfoList, WORD& type, BYTE& status, INT64& accountNo );
	void CS_CHAT_REQ_SECTOR_MOVE(SessionInfo sessionInfo, WORD& type, INT64& accountNo, WORD& sectorX, WORD& sectorY );
	void CS_CHAT_REQ_SECTOR_MOVEPost(SessionInfo sessionInfo, WORD& type, INT64& accountNo, WORD& sectorX, WORD& sectorY );
	void CS_CHAT_REQ_SECTOR_MOVE(list<SessionInfo>& sessionInfoList, WORD& type, INT64& accountNo, WORD& sectorX, WORD& sectorY );
	void CS_CHAT_REQ_SECTOR_MOVEPost(list<SessionInfo>& sessionInfoList, WORD& type, INT64& accountNo, WORD& sectorX, WORD& sectorY );
	void CS_CHAT_RES_SECTOR_MOVE(SessionInfo sessionInfo, WORD& type, INT64& accountNo, WORD& sectorX, WORD& sectorY );
	void CS_CHAT_RES_SECTOR_MOVEPost(SessionInfo sessionInfo, WORD& type, INT64& accountNo, WORD& sectorX, WORD& sectorY );
	void CS_CHAT_RES_SECTOR_MOVE(list<SessionInfo>& sessionInfoList, WORD& type, INT64& accountNo, WORD& sectorX, WORD& sectorY );
	void CS_CHAT_RES_SECTOR_MOVEPost(list<SessionInfo>& sessionInfoList, WORD& type, INT64& accountNo, WORD& sectorX, WORD& sectorY );
	void CS_CHAT_REQ_MESSAGE(SessionInfo sessionInfo, WORD& type, INT64& accountNo, WORD& messageLen, & WCHAR );
	void CS_CHAT_REQ_MESSAGEPost(SessionInfo sessionInfo, WORD& type, INT64& accountNo, WORD& messageLen, & WCHAR );
	void CS_CHAT_REQ_MESSAGE(list<SessionInfo>& sessionInfoList, WORD& type, INT64& accountNo, WORD& messageLen, & WCHAR );
	void CS_CHAT_REQ_MESSAGEPost(list<SessionInfo>& sessionInfoList, WORD& type, INT64& accountNo, WORD& messageLen, & WCHAR );
	void CS_CHAT_RES_MESSAGE(SessionInfo sessionInfo, WORD& type, INT64& accountNo, WCHAR& id[20], WCHAR& nickName[20], WORD& messageLen, WString& message );
	void CS_CHAT_RES_MESSAGEPost(SessionInfo sessionInfo, WORD& type, INT64& accountNo, WCHAR& id[20], WCHAR& nickName[20], WORD& messageLen, WString& message );
	void CS_CHAT_RES_MESSAGE(list<SessionInfo>& sessionInfoList, WORD& type, INT64& accountNo, WCHAR& id[20], WCHAR& nickName[20], WORD& messageLen, WString& message );
	void CS_CHAT_RES_MESSAGEPost(list<SessionInfo>& sessionInfoList, WORD& type, INT64& accountNo, WCHAR& id[20], WCHAR& nickName[20], WORD& messageLen, WString& message );
	void CS_CHAT_REQ_HEARTBEAT(SessionInfo sessionInfo, WORD& type );
	void CS_CHAT_REQ_HEARTBEATPost(SessionInfo sessionInfo, WORD& type );
	void CS_CHAT_REQ_HEARTBEAT(list<SessionInfo>& sessionInfoList, WORD& type );
	void CS_CHAT_REQ_HEARTBEATPost(list<SessionInfo>& sessionInfoList, WORD& type );
	testProxy(IOCPServer* pServer)
	{
		_pServer=pServer;
	}
};
