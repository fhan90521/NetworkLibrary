#pragma once
#define _CRT_SECURE_NO_WARNINGS 
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
class Log
{
private:
    inline static int _logLevel = 0;
public:
    const inline static int DEBUG_LEVEL = 0;
    const inline static int ERROR_LEVEL = 1;
    const inline static int SYSTEM_LEVEL = 2;
    static void SetLogLevel(const int logLevel)
    {
        _logLevel = logLevel;
    }
    static void Printf(const int logLevel, const char* fmt, ...)
    {
        if (_logLevel <= logLevel)
        {
            char buf[1024];
            va_list ap;
            va_start(ap, fmt);
            vsprintf_s(buf, fmt, ap);
            va_end(ap);
            puts(buf);
        }
    }
};