#pragma once
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include<iostream>
#include<stdio.h>
#include "MyWindow.h"
#define dfLOG_LEVEL_DEBUG 0
#define dfLOG_LEVEL_ERROR 1
#define dfLOG_LEVEL_SYSTEM 2

int g_iLogLevel= dfLOG_LEVEL_DEBUG;
char g_LogBuf[1024];


#define _LOG(LogLevel, fmt, ...)\
do{\
	if(g_iLogLevel<=LogLevel)\
	{\
		sprintf_s(g_LogBuf,fmt,##__VA_ARGS__);\
		std::cout<<g_LogBuf;\
	}\
}while(0)
