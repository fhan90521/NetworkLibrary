#include "RedisManager.h"
#include "Log.h"
#include "GetMyThreadID.h"
#include "ParseJson.h"
cpp_redis::client* RedisManager::GetRedisConnection()
{
	RedisConnection& redisConnection=_redisConnections[GetMyThreadID()];
	if (redisConnection.connection == nullptr)
	{
		redisConnection.connection = new cpp_redis::client;
		redisConnection.connection->connect(REDIS_IP,REDIS_PORT);
	}
	return redisConnection.connection;
}

RedisManager::RedisManager(std::string RedisSetFile, int maxThreadCnt)
{
	GetRedisSetValue(RedisSetFile);
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		int error = WSAGetLastError();
		Log::LogOnFile(Log::SYSTEM_LEVEL, "WSAStartup() error : %d\n", error);
		DebugBreak();
	}
	_maxConnection = maxThreadCnt;
	_redisConnections = new RedisConnection[_maxConnection];
}

RedisManager::~RedisManager()
{
	delete[] _redisConnections;
	WSACleanup();
}

void RedisManager::GetRedisSetValue(std::string RedisSetFile)
{
	Document RedisSetValues=ParseJson(RedisSetFile);
	REDIS_IP = RedisSetValues["REDIS_IP"].GetString();
}
