#pragma once
#include "Session.h"
#include "CMirrorBuffer.h"
#include "testPKT_TYPE.h"
class testStub
{
public:
	bool PacketProcCS_CHAT_REQ_LOGIN( SessionInfo sessionInfo, CMirrorBuffer& buf);
	virtual void ProcCS_CHAT_REQ_LOGIN( SessionInfo sessionInfo,  INT64& accountNo, WCHAR& id[20], WCHAR& nickName[20], char& sessionKey[64] ){}

	bool PacketProcCS_CHAT_RES_LOGIN( SessionInfo sessionInfo, CMirrorBuffer& buf);
	virtual void ProcCS_CHAT_RES_LOGIN( SessionInfo sessionInfo,  WORD& type, BYTE& status, INT64& accountNo ){}

	bool PacketProcCS_CHAT_REQ_SECTOR_MOVE( SessionInfo sessionInfo, CMirrorBuffer& buf);
	virtual void ProcCS_CHAT_REQ_SECTOR_MOVE( SessionInfo sessionInfo,  WORD& type, INT64& accountNo, WORD& sectorX, WORD& sectorY ){}

	bool PacketProcCS_CHAT_RES_SECTOR_MOVE( SessionInfo sessionInfo, CMirrorBuffer& buf);
	virtual void ProcCS_CHAT_RES_SECTOR_MOVE( SessionInfo sessionInfo,  WORD& type, INT64& accountNo, WORD& sectorX, WORD& sectorY ){}

	bool PacketProcCS_CHAT_REQ_MESSAGE( SessionInfo sessionInfo, CMirrorBuffer& buf);
	virtual void ProcCS_CHAT_REQ_MESSAGE( SessionInfo sessionInfo,  WORD& type, INT64& accountNo, WORD& messageLen, & WCHAR ){}

	bool PacketProcCS_CHAT_RES_MESSAGE( SessionInfo sessionInfo, CMirrorBuffer& buf);
	virtual void ProcCS_CHAT_RES_MESSAGE( SessionInfo sessionInfo,  WORD& type, INT64& accountNo, WCHAR& id[20], WCHAR& nickName[20], WORD& messageLen, WString& message ){}

	bool PacketProcCS_CHAT_REQ_HEARTBEAT( SessionInfo sessionInfo, CMirrorBuffer& buf);
	virtual void ProcCS_CHAT_REQ_HEARTBEAT( SessionInfo sessionInfo,  WORD& type ){}

	bool PacketProc(SessionInfo sessionInfo, CMirrorBuffer& buf);
};
