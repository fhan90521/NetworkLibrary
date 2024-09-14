#include "MYSQLHelper.h"
#include "rapidjson/ParseJson.h"
#include "DebugTool/Log.h"
MYSQL* MYSQLHelper::GetMYSQL()
{
	return &GetConnectionRef().connection;
}
bool MYSQLHelper::Connect()
{
	return GetConnectionRef().isConnecting;
}
MYSQLHelper::MYSQLHelper(std::string DBSetFile)
{
	GetDBSetValue(DBSetFile);
	_tlsIndex = TlsAlloc();
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

bool MYSQLHelper::Connect(MYSQLConnection& dbConnection)
{
	bool ret = true;
	EXCLUSIVE_LOCK;
	mysql_init(&dbConnection.connection);
	dbConnection.isConnecting = mysql_real_connect(&dbConnection.connection, DB_IP.data(), DB_USER.data(), DB_PASSWORD.data(), DB_SCHEMA.data(), DB_PORT, (char*)NULL, 0);
	if (dbConnection.isConnecting == NULL)
	{
		ret = false;
		Log::LogOnFile(Log::SYSTEM_LEVEL, "Mysql connection error : % s", mysql_error(&dbConnection.connection));
	}
	return ret;
}

MYSQLHelper::MYSQLConnection& MYSQLHelper::GetConnectionRef()
{
	// TODO: 여기에 return 문을 삽입합니다.
	MYSQLConnection* pConnection = (MYSQLConnection*)TlsGetValue(_tlsIndex);
	if (pConnection == nullptr)
	{
		pConnection = (MYSQLConnection*)_aligned_malloc(sizeof(MYSQLConnection), 64);
		new (pConnection) MYSQLConnection;
		TlsSetValue(_tlsIndex, pConnection);
	}
	if (pConnection->isConnecting == NULL)
	{
		Connect(*pConnection);
	}

	return *pConnection;
}

void MYSQLHelper::CloseConnection()
{
	MYSQLConnection& dbConnection = GetConnectionRef();
	mysql_close(&dbConnection.connection);
	dbConnection.isConnecting = NULL;
}

bool MYSQLHelper::SendQuery(const char* query, MYSQL_BIND* binds)
{
	MYSQL_STMT* stmt = mysql_stmt_init(&GetConnectionRef().connection);
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


