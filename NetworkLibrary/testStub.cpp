#include  "testStub.h"
 #include "IOCPServer.h"
#include <iostream>
using namespace std;
bool testStub::PacketProcCS_CHAT_REQ_LOGIN(SessionInfo sessionInfo, CMirrorBuffer& buf)
{
	INT64 accountNo;
	WCHAR id[20];
	WCHAR nickName[20];
	char sessionKey[64];
	try
	{
		buf >> accountNo >> id[20] >> nickName[20] >> sessionKey[64];
	}
	catch(int useSize)
	{
		 cout<<" PacketProcCS_CHAT_REQ_LOGIN error"<<endl;
		 return false;
	}
	ProcCS_CHAT_REQ_LOGIN( sessionInfo, accountNo, id[20], nickName[20], sessionKey[64]);
	return true;
}
bool testStub::PacketProcCS_CHAT_RES_LOGIN(SessionInfo sessionInfo, CMirrorBuffer& buf)
{
	WORD type;
	BYTE status;
	INT64 accountNo;
	try
	{
		buf >> type >> status >> accountNo;
	}
	catch(int useSize)
	{
		 cout<<" PacketProcCS_CHAT_RES_LOGIN error"<<endl;
		 return false;
	}
	ProcCS_CHAT_RES_LOGIN( sessionInfo, type, status, accountNo);
	return true;
}
bool testStub::PacketProcCS_CHAT_REQ_SECTOR_MOVE(SessionInfo sessionInfo, CMirrorBuffer& buf)
{
	WORD type;
	INT64 accountNo;
	WORD sectorX;
	WORD sectorY;
	try
	{
		buf >> type >> accountNo >> sectorX >> sectorY;
	}
	catch(int useSize)
	{
		 cout<<" PacketProcCS_CHAT_REQ_SECTOR_MOVE error"<<endl;
		 return false;
	}
	ProcCS_CHAT_REQ_SECTOR_MOVE( sessionInfo, type, accountNo, sectorX, sectorY);
	return true;
}
bool testStub::PacketProcCS_CHAT_RES_SECTOR_MOVE(SessionInfo sessionInfo, CMirrorBuffer& buf)
{
	WORD type;
	INT64 accountNo;
	WORD sectorX;
	WORD sectorY;
	try
	{
		buf >> type >> accountNo >> sectorX >> sectorY;
	}
	catch(int useSize)
	{
		 cout<<" PacketProcCS_CHAT_RES_SECTOR_MOVE error"<<endl;
		 return false;
	}
	ProcCS_CHAT_RES_SECTOR_MOVE( sessionInfo, type, accountNo, sectorX, sectorY);
	return true;
}
bool testStub::PacketProcCS_CHAT_REQ_MESSAGE(SessionInfo sessionInfo, CMirrorBuffer& buf)
{
	WORD type;
	INT64 accountNo;
	WORD messageLen;
	 WCHAR;
	try
	{
		buf >> type >> accountNo >> messageLen >> WCHAR;
	}
	catch(int useSize)
	{
		 cout<<" PacketProcCS_CHAT_REQ_MESSAGE error"<<endl;
		 return false;
	}
	ProcCS_CHAT_REQ_MESSAGE( sessionInfo, type, accountNo, messageLen, WCHAR);
	return true;
}
bool testStub::PacketProcCS_CHAT_RES_MESSAGE(SessionInfo sessionInfo, CMirrorBuffer& buf)
{
	WORD type;
	INT64 accountNo;
	WCHAR id[20];
	WCHAR nickName[20];
	WORD messageLen;
	WString message;
	try
	{
		buf >> type >> accountNo >> id[20] >> nickName[20] >> messageLen >> message;
	}
	catch(int useSize)
	{
		 cout<<" PacketProcCS_CHAT_RES_MESSAGE error"<<endl;
		 return false;
	}
	ProcCS_CHAT_RES_MESSAGE( sessionInfo, type, accountNo, id[20], nickName[20], messageLen, message);
	return true;
}
bool testStub::PacketProcCS_CHAT_REQ_HEARTBEAT(SessionInfo sessionInfo, CMirrorBuffer& buf)
{
	WORD type;
	try
	{
		buf >> type;
	}
	catch(int useSize)
	{
		 cout<<" PacketProcCS_CHAT_REQ_HEARTBEAT error"<<endl;
		 return false;
	}
	ProcCS_CHAT_REQ_HEARTBEAT( sessionInfo, type);
	return true;
}

bool testStub::PacketProc(SessionInfo sessionInfo, CMirrorBuffer& buf)
{
	 short packetType;
	buf>>packetType;
	switch(packetType)
	{
	case PKT_TYPE_CS_CHAT_REQ_LOGIN:
	{
		return PacketProcCS_CHAT_REQ_LOGIN(sessionInfo,buf);
		break;
	}
	case PKT_TYPE_CS_CHAT_RES_LOGIN:
	{
		return PacketProcCS_CHAT_RES_LOGIN(sessionInfo,buf);
		break;
	}
	case PKT_TYPE_CS_CHAT_REQ_SECTOR_MOVE:
	{
		return PacketProcCS_CHAT_REQ_SECTOR_MOVE(sessionInfo,buf);
		break;
	}
	case PKT_TYPE_CS_CHAT_RES_SECTOR_MOVE:
	{
		return PacketProcCS_CHAT_RES_SECTOR_MOVE(sessionInfo,buf);
		break;
	}
	case PKT_TYPE_CS_CHAT_REQ_MESSAGE:
	{
		return PacketProcCS_CHAT_REQ_MESSAGE(sessionInfo,buf);
		break;
	}
	case PKT_TYPE_CS_CHAT_RES_MESSAGE:
	{
		return PacketProcCS_CHAT_RES_MESSAGE(sessionInfo,buf);
		break;
	}
	case PKT_TYPE_CS_CHAT_REQ_HEARTBEAT:
	{
		return PacketProcCS_CHAT_REQ_HEARTBEAT(sessionInfo,buf);
		break;
	}
	default:
	{
		cout<<"Packet Type not exist error"<<endl;
		return false;
		break;
	}
	}
}
