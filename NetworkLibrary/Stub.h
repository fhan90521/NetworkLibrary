#pragma once
#include "Session.h"
#include "CRingBuffer.h"
#include "PKT_TYPE.h"
class Stub
{
public:
	bool PacketProcEcho( SessionInfo sessionInfo, CRingBuffer& buf);
	virtual void ProcEcho( SessionInfo sessionInfo,  long long& testLL ){}

	bool PacketProc(SessionInfo sessionInfo, PKT_TYPE packetType, CRingBuffer& buf);
};
