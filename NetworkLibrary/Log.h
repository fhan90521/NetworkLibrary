#pragma once
#define _CRT_SECURE_NO_WARNINGS 
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctime>
#include <fstream>
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
    static void LogOnConsole(const int logLevel, const char* fmt, ...)
    {
        if (_logLevel <= logLevel)
        {
            char buf[512];
            va_list ap;
            va_start(ap, fmt);
            vsprintf_s(buf, fmt, ap);
            va_end(ap);
            puts(buf);
        }
    }
    static void LogOnFile(const int logLevel, const char* fmt, ...)
    {
      
        if (_logLevel <= logLevel)
        {
            time_t now = time(0);
            tm localTime;
            errno_t error = localtime_s(&localTime,&now);
           

            // 날짜 정보 추출
            int year = 1900 + localTime.tm_year;
            int month = 1 + localTime.tm_mon;
            int day = localTime.tm_mday;

            // 파일 이름 생성 (예: "2024-02-13.txt")
            std::string filename = std::to_string(year) + "-" + std::to_string(month) + "-" + std::to_string(day) + "-log.txt";

            // 파일 출력 객체 생성
            std::ofstream fout(filename,std::ios::app);
            char buf[512];
            va_list ap;
            va_start(ap, fmt);
            vsprintf_s(buf, fmt, ap);
            va_end(ap);
            fout << buf;
        }
    }
};