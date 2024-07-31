#include "MYSQLHelper.h"
#include "GetMyThreadID.h"
#include "ParseJson.h"
MYSQLHelper::MYSQLHelper(std::string DBSetFile, int maxThreadCnt)
{
	if (InterlockedExchange8(&_bInitialLock, true) == false)
	{
		InitializeSRWLock(&_DBInitialLock);
	}
	GetDBSetValue(DBSetFile);
	_maxThreadCnt = maxThreadCnt;
	_MYSQLConnections = new MYSQLConnection[_maxThreadCnt];
}
MYSQLHelper::~MYSQLHelper()
{
	delete[] _MYSQLConnections;
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
	AcquireSRWLockExclusive(&_DBInitialLock);
	MYSQLConnection& dbConnection = _MYSQLConnections[GetMyThreadID()];
	mysql_init(&dbConnection.connection);
	dbConnection.isConnecting = mysql_real_connect(&dbConnection.connection, DB_IP.data(), DB_USER.data(), DB_PASSWORD.data(), DB_SCHEMA.data(), DB_PORT, (char*)NULL, 0);
	if (dbConnection.isConnecting == NULL)
	{
		ret = false;
		Log::LogOnFile(Log::SYSTEM_LEVEL, "Mysql connection error : % s", mysql_error(&dbConnection.connection));
	}
	ReleaseSRWLockExclusive(&_DBInitialLock);
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


