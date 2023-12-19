#pragma once
#include "Session.h"
#include "CMirrorBuffer.h"
#include "PKT_TYPE.h"
class Stub
{
public:
	bool PacketProcEcho( SessionInfo sessionInfo, CMirrorBuffer& buf);
	virtual void ProcEcho( SessionInfo sessionInfo,  long long& testLL ){}

	bool PacketProc(SessionInfo sessionInfo, PKT_TYPE packetType, CMirrorBuffer& buf);
};
