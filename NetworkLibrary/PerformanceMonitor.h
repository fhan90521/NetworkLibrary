#pragma once
#pragma comment(lib, "IPHLPAPI.lib")
#include "MyWindow.h"
#include <psapi.h>
#include <set>
#include <string>
#include <iphlpapi.h>
#include "Log.h"
#include <stdlib.h>
class PerformanceMonitor
{
public:
	PerformanceMonitor(HANDLE hProcess = INVALID_HANDLE_VALUE);
	
	void UpdateMonitorData(void);

	float GetSystemCpuTotal(void) { return _systemCpuTotal; }
	float GetSystemCpuUser(void) { return _systemCpuUser; }
	float GetPSystemCpuKernel(void) { return _systemCpuKernel; }

	float GetProcessCpuTotal(void) { return _processCpuTotal; }
	float GetProcessCpuUser(void) { return _processCpuUser; }
	float GetProcessCpuKernel(void) { return _processCpuKernel; }

	float GetProcessUserMemoryByMB() { return (float)_pmc.PrivateUsage/1000000; }
	float GetProcessNonPagedByMB() { return (float)_pmc.QuotaNonPagedPoolUsage/ 1000000; }
	float GetSystemNonPagedByMB() { return (float)_perfInfo.KernelNonpaged*4096/1000000; }
	float GetSystemAvailMemoryByGB() { return (float)_perfInfo.PhysicalAvailable*4096/1000000000; }

	int GetOutDataSizeByKB() 
	{ 
		int ret = _outDataOct - _prevOut;
		if (ret >= 0)
		{
			_lastNormalOut = ret;
			return ret;
		}
		return _lastNormalOut;
	}
	int GetInDataSizeByKB() 
	{
		int ret = _inDataOct - _prevIn;
		if (ret >= 0)
		{
			_lastNormalIn = ret;
			return ret;
		}
		return _lastNormalIn;
	}
	void AddInterface(std::string interfaceIp);

	void PrintMonitorData();
private:
	HANDLE _hProcess;
	
	//cpu 사용량
	int _iNumberOfProcessors;

	float    _systemCpuTotal;
	float    _systemCpuUser;
	float    _systemCpuKernel;

	float _processCpuTotal;
	float _processCpuUser;
	float _processCpuKernel;

	ULARGE_INTEGER _systemLastTimeKernel;
	ULARGE_INTEGER _systemLastTimeUser;
	ULARGE_INTEGER _systemLastTimeIdle;

	ULARGE_INTEGER _processLastTimeKernel;
	ULARGE_INTEGER _processLastTimeUser;
	ULARGE_INTEGER _processLastTimeTotal;
	
	//메모리 사용량
	PERFORMANCE_INFORMATION _perfInfo;
	PROCESS_MEMORY_COUNTERS_EX _pmc;

	//네트워크 사용량
	std::set<int> _interfaceIndexSet;

	double _inDataOct=0;
	double _outDataOct=0;
	double  _prevOut = 0;
	double _prevIn = 0;
	int _lastNormalOut = 0;
	int _lastNormalIn = 0;
};