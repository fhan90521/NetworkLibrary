#pragma once
#pragma comment (lib, "cpp_redis.lib")
#pragma comment (lib, "tacopie.lib")
#include <cpp_redis/cpp_redis>
#include <string>
class RedisManager
{
private:
	struct RedisConnection
	{
		cpp_redis::client* connection = nullptr;
	};
	RedisConnection* _redisConnections;
	std::string REDIS_IP;
	unsigned int REDIS_PORT = 6379;
	int _maxConnection;
	void GetRedisSetValue(std::string RedisSetFile);
public:
	RedisManager(std::string RedisSetFile, int maxThreadCnt = 64);
	~RedisManager();
	cpp_redis::client* GetRedisConnection();
};