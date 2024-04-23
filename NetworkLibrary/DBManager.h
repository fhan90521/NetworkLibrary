#pragma once
#pragma comment(lib,"mysqlclient.lib")
#include "MyWindow.h"
#include <string>
#include "include/mysql.h"
#include "include/errmsg.h"
#include "Log.h"
class DBManager
{
	//DB
private:
	struct DBConnection
	{
		MYSQL connection;
		MYSQL* isConnecting = NULL;
	};
	std::string DB_IP;
	std::string DB_USER;
	std::string DB_PASSWORD;
	std::string DB_SCHEMA;
	unsigned int DB_PORT = 3306;
	int _maxConnection;
	DBConnection* _DBConnections;
	inline static SRWLOCK _DBInitialLock;
	inline static char _bInitialLock = false;
public:
	DBManager(std::string DBSetFile,int maxThreadCnt=64);
	~DBManager();
	void GetDBSetValue(std::string DBSetFile);
	bool ConnectDB();
	MYSQL* GetDBConnection();
	void CloseDBConnection();
	static void MakeQuery(char* dest, int destSize, const char* fmt, ...)
	{
		va_list ap;
		va_start(ap, fmt);
		vsprintf_s(dest, destSize, fmt, ap);
		va_end(ap);
	}
};