#pragma once
#pragma comment (lib, "cpp_redis.lib")
#pragma comment (lib, "tacopie.lib")
#include <cpp_redis/cpp_redis>
#include <string>
class RedisHelper
{
private:
	cpp_redis::client _connection;
	std::string REDIS_IP;
	unsigned int REDIS_PORT = 6379;
	int _maxThreadCnt;
	void GetRedisSetValue(std::string RedisSetFile);
public:
	RedisHelper(std::string RedisSetFile);
	~RedisHelper();
	cpp_redis::client* GetRedisConnection();
};