#pragma once
#pragma comment(lib,"mysql/mysqlclient.lib")
#include "OS/MyWindow.h"
#include "Container/MyStlContainer.h"
#include "Lock/LockGuard.h"
#include "mysql/include/mysql.h"
#include "mysql/include/errmsg.h"
#include <type_traits>
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
    int _tlsIndex;
	inline static USE_LOCK;
    MYSQLConnection& GetConnectionRef();
    bool Connect(MYSQLConnection& connection);
public:
    MYSQL* GetMYSQL();
    bool Connect();
    MYSQLHelper(std::string DBSetFile);
	~MYSQLHelper();
	void GetDBSetValue(std::string DBSetFile);
	void CloseConnection();
	bool SendQuery(const char* query, MYSQL_BIND* parameters);
    template<typename... Args>
    static void InitBind(MYSQL_BIND* binds, bool* isNulls, Args&... args) {
        InitBindHelper(binds, isNulls, args...);
    }
private:
    template<typename T>
    class TypeTracer;

    template<typename T>
    static void InitBindHelper(MYSQL_BIND& bind, T& value, bool* isNull) {
        memset(&bind, 0, sizeof(MYSQL_BIND));
        if constexpr (std::is_same_v<T, int>) {
            bind.buffer_type = MYSQL_TYPE_LONG;
            bind.buffer = (char*)&value;
        }
        else if constexpr (std::is_same_v<T, unsigned int>) {
            bind.buffer_type = MYSQL_TYPE_LONG;
            bind.buffer = (char*)&value;
        }
        else if constexpr (std::is_same_v<T, long>) {
            bind.buffer_type = MYSQL_TYPE_LONG;
            bind.buffer = (char*)&value;
        }
        else if constexpr (std::is_same_v<T, unsigned long>) {
            bind.buffer_type = MYSQL_TYPE_LONG;
            bind.buffer = (char*)&value;
        }
        else if constexpr (std::is_same_v<T, long long>) {
            bind.buffer_type = MYSQL_TYPE_LONGLONG;
            bind.buffer = (char*)&value;
        }
        else if constexpr (std::is_same_v<T, unsigned long long>) {
            bind.buffer_type = MYSQL_TYPE_LONGLONG;
            bind.buffer = (char*)&value;
        }
        else if constexpr (std::is_same_v<T, float>) {
            bind.buffer_type = MYSQL_TYPE_FLOAT;
            bind.buffer = (char*)&value;
        }
        else if constexpr (std::is_same_v<T, double>) {
            bind.buffer_type = MYSQL_TYPE_DOUBLE;
            bind.buffer = (char*)&value;
        }
        else if constexpr (std::is_same_v<T, bool>) {
            bind.buffer_type = MYSQL_TYPE_BOOL;
            bind.buffer = (char*)&value;
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            bind.buffer_type = MYSQL_TYPE_STRING;
            bind.buffer = (char*)value.c_str();
            bind.buffer_length = value.size();
            bind.length = &bind.buffer_length;
        }
        else {
            TypeTracer<decltype(value)> tt;
        }

        bind.is_null = isNull;
    }

    template<typename T>
    static void InitBindHelper(MYSQL_BIND* binds, bool* isNulls, T& firstArg) {
        InitBindHelper(binds[0], firstArg, &isNulls[0]);
    }

    template<typename T, typename... Rest>
    static void InitBindHelper(MYSQL_BIND* binds, bool* isNulls, T& firstArg, Rest&... restArgs) {
        InitBindHelper(binds[0], firstArg, &isNulls[0]);
        InitBindHelper(binds+1, isNulls+1, restArgs...);
    }
};