#pragma once
#define WAN_HEADER_SIZE 2
#define LAN_HEADER_SIZE 2
#define DIFF_HEADER_SIZE 0
#pragma pack(1)
struct NetHeader
{
	short len;
};
#pragma pack()