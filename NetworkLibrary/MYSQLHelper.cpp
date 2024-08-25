#include "MYSQLHelper.h"
#include "GetMyThreadID.h"
#include "ParseJson.h"
#include "Log.h"
MYSQLHelper::MYSQLHelper(std::string DBSetFile, int maxThreadCnt)
{
	GetDBSetValue(DBSetFile);
	_maxThreadCnt = maxThreadCnt;
	_MYSQLConnections = new MYSQLConnection[_maxThreadCnt];
}
MYSQLHelper::~MYSQLHelper()
{
	// 정적으로 사용해서 굳이 정리 안함
}
void MYSQLHelper::GetDBSetValue(std::string DBSetFile)
{
	Document DBSetValues=ParseJson(DBSetFile);
	DB_IP = DBSetValues["DB_IP"].GetString();
	DB_USER = DBSetValues["DB_USER"].GetString();
	DB_PASSWORD = DBSetValues["DB_PASSWORD"].GetString();
	DB_SCHEMA = DBSetValues["DB_SCHEMA"].GetString();
}

bool MYSQLHelper::Connect()
{
	bool ret = true;
	EXCLUSIVE_LOCK;
	MYSQLConnection& dbConnection = _MYSQLConnections[GetMyThreadID()];
	mysql_init(&dbConnection.connection);
	dbConnection.isConnecting = mysql_real_connect(&dbConnection.connection, DB_IP.data(), DB_USER.data(), DB_PASSWORD.data(), DB_SCHEMA.data(), DB_PORT, (char*)NULL, 0);
	if (dbConnection.isConnecting == NULL)
	{
		ret = false;
		Log::LogOnFile(Log::SYSTEM_LEVEL, "Mysql connection error : % s", mysql_error(&dbConnection.connection));
	}
	return ret;
}

MYSQL* MYSQLHelper::GetConnection()
{
	MYSQLConnection& dbConnection = _MYSQLConnections[GetMyThreadID()];
	
	if (dbConnection.isConnecting == NULL)
	{
		Connect();
	}
	return &dbConnection.connection;
}

void MYSQLHelper::CloseConnection()
{
	MYSQLConnection& dbConnection = _MYSQLConnections[GetMyThreadID()];
	mysql_close(&dbConnection.connection);
	dbConnection.isConnecting = NULL;
}

bool MYSQLHelper::SendQuery(const char* query, MYSQL_BIND* binds)
{
	MYSQL_STMT* stmt = mysql_stmt_init(GetConnection());
	if (stmt == nullptr)
	{
		Log::LogOnFile(Log::SYSTEM_LEVEL, "stmt_init error\n");
		return false;
	}
	if (mysql_stmt_prepare(stmt, query, strlen(query)) != 0)
	{
		Log::LogOnFile(Log::SYSTEM_LEVEL, "stmt prepare error\n");
		mysql_stmt_close(stmt);
		return false;
	}
	
	if (mysql_stmt_bind_param(stmt, binds) != 0 )
	{
		Log::LogOnFile(Log::SYSTEM_LEVEL, "stmt bind error &s\n",mysql_stmt_error(stmt));
		mysql_stmt_close(stmt);
		return false;
	}

	if (mysql_stmt_execute(stmt) != 0)
	{
		Log::LogOnFile(Log::SYSTEM_LEVEL, "stmt excute error %s\n", mysql_stmt_error(stmt));
		mysql_stmt_close(stmt);
		return false;
	}

	mysql_stmt_close(stmt);
	return true;
}


