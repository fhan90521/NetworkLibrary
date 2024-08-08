#include "RedisHelper.h"
#include "Log.h"
#include "GetMyThreadID.h"
#include "ParseJson.h"
cpp_redis::client* RedisHelper::GetRedisConnection()
{
	RedisConnection& redisConnection=_redisConnections[GetMyThreadID()];
	if (redisConnection.connection == nullptr)
	{
		redisConnection.connection = new cpp_redis::client;
		redisConnection.connection->connect(REDIS_IP,REDIS_PORT);
	}
	return redisConnection.connection;
}

RedisHelper::RedisHelper(std::string RedisSetFile, int maxThreadCnt)
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

	_maxThreadCnt = maxThreadCnt;
	_redisConnections = new RedisConnection[_maxThreadCnt];
}

RedisHelper::~RedisHelper()
{
	delete[] _redisConnections;
	WSACleanup();
}

void RedisHelper::GetRedisSetValue(std::string RedisSetFile)
{
	Document RedisSetValues=ParseJson(RedisSetFile);
	REDIS_IP = RedisSetValues["REDIS_IP"].GetString();
}
