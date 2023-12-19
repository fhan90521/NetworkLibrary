#include "Stub.h"
#include "IOCPServer.h"
#include <iostream>
using namespace std;
bool Stub::PacketProcEcho(SessionInfo sessionInfo, CMirrorBuffer& buf)
{
	long long testLL;
	try
	{
		buf >> testLL;
	}
	catch(int useSize)
	{
		 cout<<" PacketProcEcho error"<<endl;
		 return false;
	}
	ProcEcho( sessionInfo, testLL);
	return true;
}

bool Stub::PacketProc(SessionInfo sessionInfo, PKT_TYPE packetType, CMirrorBuffer& buf)
{
	switch(packetType)
	{
	case PKT_TYPE_Echo:
	{
		return PacketProcEcho(sessionInfo,buf);
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
