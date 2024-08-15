#pragma once
#pragma comment(lib,"mysqlclient.lib")
#include "MyWindow.h"
#include <string>
#include "include/mysql.h"
#include "include/errmsg.h"
#include "LockGuard.h"
class MYSQLHelper
{
	//DB
private:
	struct MYSQLConnection
	{
		MYSQL connection;
		MYSQL* isConnecting = NULL;
	};
	std::string DB_IP;
	std::string DB_USER;
	std::string DB_PASSWORD;
	std::string DB_SCHEMA;
	unsigned int DB_PORT = 3306;
	int _maxThreadCnt;
	MYSQLConnection* _MYSQLConnections;
	inline static USE_LOCK;
public:
	MYSQLHelper(std::string DBSetFile,int maxThreadCnt=64);
	~MYSQLHelper();
	void GetDBSetValue(std::string DBSetFile);
	bool Connect();
	MYSQL* GetConnection();
	void CloseConnection();
	static void MakeQuery(char* dest, int destSize, const char* fmt, ...)
	{
		va_list ap;
		va_start(ap, fmt);
		vsprintf_s(dest, destSize, fmt, ap);
		va_end(ap);
	}
};