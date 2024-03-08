#pragma once
#define _CRT_SECURE_NO_WARNINGS 
#include "include/mysql.h"
#include "include/errmsg.h"
#include "Log.h"
#include "MyWindow.h"
#include <stdarg.h>
class DBJob
{	
public:
	DBJob() {}
	virtual ~DBJob() {}
	virtual void Execute(MYSQL* connection) = 0;
    static void MakeQuery(char* query,int querySize ,const char* fmt, ...)
    {
        va_list ap;
        va_start(ap, fmt);
        vsprintf_s(query, querySize,fmt, ap);
        va_end(ap);
    }
};