#include "RedisManager.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"
#include "Log.h"
#include "GetMyThreadID.h"
using namespace rapidjson;
cpp_redis::client* RedisManager::GetRedisConnection()
{
	RedisConnection& redisConnection=_redisConnections[GetMyThreadID()];
	if (redisConnection.connection == nullptr)
	{
		redisConnection.connection = new cpp_redis::client;
		redisConnection.connection->connect();
	}
	return redisConnection.connection;
}

RedisManager::RedisManager(std::string RedisSetFile, int maxThreadCnt)
{
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
	Document RedisSetValues;
	std::ifstream fin(RedisSetFile);
	if (!fin)
	{
		Log::LogOnFile(Log::SYSTEM_LEVEL, "there is no %s\n", RedisSetFile.data());
		DebugBreak();
	}
	std::string json((std::istreambuf_iterator<char>(fin)), (std::istreambuf_iterator<char>()));
	fin.close();
	rapidjson::ParseResult parseResult = RedisSetValues.Parse(json.data());
	if (!parseResult) {
		fprintf(stderr, "JSON parse error: %s (%d)", GetParseError_En(parseResult.Code()), parseResult.Offset());
		exit(EXIT_FAILURE);
	}
	REDIS_IP = RedisSetValues["REDIS_IP"].GetString();
}
