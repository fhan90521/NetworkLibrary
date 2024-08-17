#include "RedisHelper.h"
#include "Log.h"
#include "ParseJson.h"
cpp_redis::client* RedisHelper::GetRedisConnection()
{
	return &_connection;
}

RedisHelper::RedisHelper(std::string RedisSetFile)
{
	GetRedisSetValue(RedisSetFile);

	//IOCPServer와 독립적으로 쓸 때 필요
	/*WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		int error = WSAGetLastError();
		Log::LogOnFile(Log::SYSTEM_LEVEL, "WSAStartup() error : %d\n", error);
		DebugBreak();
	}*/
	_connection.connect(REDIS_IP, REDIS_PORT);
}

RedisHelper::~RedisHelper()
{
}

void RedisHelper::GetRedisSetValue(std::string RedisSetFile)
{
	Document RedisSetValues=ParseJson(RedisSetFile);
	REDIS_IP = RedisSetValues["REDIS_IP"].GetString();
}
