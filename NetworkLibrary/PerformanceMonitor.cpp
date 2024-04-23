#include"PerformanceMonitor.h"
#include <iostream>
#include <format>
PerformanceMonitor::PerformanceMonitor(HANDLE hProcess)
{
	//------------------------------------------------------------------ 
 // 프로세스 핸들 입력이 없다면 자기 자신을 대상으로... 
 //------------------------------------------------------------------ 
	if (hProcess == INVALID_HANDLE_VALUE)
	{
		_hProcess = GetCurrentProcess();
	}

	//------------------------------------------------------------------ 
	// 프로세서 개수를 확인한다. 
	// 
	// 프로세스 (exe) 실행률 계산시 cpu 개수로 나누기를 하여 실제 사용률을 구함. 
	//------------------------------------------------------------------ 
	SYSTEM_INFO SystemInfo;

	GetSystemInfo(&SystemInfo);

	//cpu
	_iNumberOfProcessors = SystemInfo.dwNumberOfProcessors;

	_systemCpuTotal = 0;
	_systemCpuUser = 0;
	_systemCpuKernel = 0;

	_processCpuTotal = 0;
	_processCpuUser = 0;
	_processCpuKernel = 0;

	_systemLastTimeKernel.QuadPart = 0;
	_systemLastTimeUser.QuadPart = 0;
	_systemLastTimeIdle.QuadPart = 0;

	_processLastTimeUser.QuadPart = 0;
	_processLastTimeKernel.QuadPart = 0;
	_processLastTimeTotal.QuadPart = 0;
	UpdateMonitorData();
}

void PerformanceMonitor::UpdateMonitorData(void)
{
	//--------------------------------------------------------- 
	// 프로세서 사용률을 갱신한다. 
	// 
	// 본래의 사용 구조체는 FILETIME 이지만, ULARGE_INTEGER 와 구조가 같으므로 이를 사용함. 
	// FILETIME 구조체는 100 나노세컨드 단위의 시간 단위를 표현하는 구조체임. 
	//--------------------------------------------------------- 
	ULARGE_INTEGER Idle;
	ULARGE_INTEGER Kernel;
	ULARGE_INTEGER User;

	//--------------------------------------------------------- 
	// 시스템 사용 시간을 구한다. 
	// 
	// 아이들 타임 / 커널 사용 타임 (아이들포함) / 유저 사용 타임 
	//--------------------------------------------------------- 
	if (GetSystemTimes((PFILETIME)&Idle, (PFILETIME)&Kernel, (PFILETIME)&User) == false)
	{
		return;
	}

	// 커널 타임에는 아이들 타임이 포함됨. 
	ULONGLONG KernelDiff = Kernel.QuadPart - _systemLastTimeKernel.QuadPart;
	ULONGLONG UserDiff = User.QuadPart - _systemLastTimeUser.QuadPart;
	ULONGLONG IdleDiff = Idle.QuadPart - _systemLastTimeIdle.QuadPart;

	ULONGLONG Total = KernelDiff + UserDiff;
	ULONGLONG TimeDiff;


	if (Total == 0)
	{
		_systemCpuUser = 0.0f;
		_systemCpuKernel = 0.0f;
		_systemCpuTotal = 0.0f;
	}
	else
	{
		// 커널 타임에 아이들 타임이 있으므로 빼서 계산. 
		_systemCpuTotal = (float)((double)(Total - IdleDiff) / Total * 100.0f);
		_systemCpuUser = (float)((double)UserDiff / Total * 100.0f);
		_systemCpuKernel = (float)((double)(KernelDiff - IdleDiff) / Total * 100.0f);
	}

	_systemLastTimeKernel = Kernel;
	_systemLastTimeUser = User;
	_systemLastTimeIdle = Idle;


	//--------------------------------------------------------- 
	// 지정된 프로세스 사용률을 갱신한다. 
	//--------------------------------------------------------- 
	ULARGE_INTEGER None;
	ULARGE_INTEGER NowTime;

	//--------------------------------------------------------- 
	// 현재의 100 나노세컨드 단위 시간을 구한다. UTC 시간. 
	// 
	// 프로세스 사용률 판단의 공식 
	// 
	// a = 샘플간격의 시스템 시간을 구함. (그냥 실제로 지나간 시간) 
	// b = 프로세스의 CPU 사용 시간을 구함. 
	// 
	// a : 100 = b : 사용률   공식으로 사용률을 구함. 
	//--------------------------------------------------------- 

	//--------------------------------------------------------- 
	// 얼마의 시간이 지났는지 100 나노세컨드 시간을 구함,  
	//--------------------------------------------------------- 
	GetSystemTimeAsFileTime((LPFILETIME)&NowTime);

	//--------------------------------------------------------- 
	// 해당 프로세스가 사용한 시간을 구함. 
	// 
	// 두번째, 세번째는 실행,종료 시간으로 미사용. 
	//--------------------------------------------------------- 
	GetProcessTimes(_hProcess, (LPFILETIME)&None, (LPFILETIME)&None, (LPFILETIME)&Kernel, (LPFILETIME)&User);

	//--------------------------------------------------------- 
	// 이전에 저장된 프로세스 시간과의 차를 구해서 실제로 얼마의 시간이 지났는지 확인. 
	// 
	// 그리고 실제 지나온 시간으로 나누면 사용률이 나옴. 
	//--------------------------------------------------------- 
	TimeDiff = NowTime.QuadPart - _processLastTimeTotal.QuadPart;
	UserDiff = User.QuadPart - _processLastTimeUser.QuadPart;
	KernelDiff = Kernel.QuadPart - _processLastTimeKernel.QuadPart;

	Total = KernelDiff + UserDiff;

	_processCpuTotal = (float)(Total / (double)_iNumberOfProcessors / (double)TimeDiff * 100.0f);
	_processCpuKernel = (float)(KernelDiff / (double)_iNumberOfProcessors / (double)TimeDiff * 100.0f);
	_processCpuUser = (float)(UserDiff / (double)_iNumberOfProcessors / (double)TimeDiff * 100.0f);

	_processLastTimeTotal = NowTime;
	_processLastTimeKernel = Kernel;
	_processLastTimeUser = User;


	//memory
	GetPerformanceInfo(&_perfInfo, sizeof(_perfInfo));
	GetProcessMemoryInfo(_hProcess, (PROCESS_MEMORY_COUNTERS*)&_pmc, sizeof(_pmc));

	//네트워크 사용량
	_prevOut = _outDataOct;
	_prevIn = _inDataOct;
	_outDataOct = 0;
	_inDataOct = 0;
	MIB_IFROW ifRow;
	for (int ifIndex : _interfaceIndexSet)
	{
		ifRow.dwIndex = ifIndex;
		
		DWORD retGetIf = GetIfEntry(&ifRow);
		if (retGetIf == NO_ERROR)
		{
			_outDataOct += ifRow.dwOutOctets / 1000;
			_inDataOct += ifRow.dwInOctets / 1000;
		}
	}
}

void PerformanceMonitor::PrintMonitorData()
{
	UpdateMonitorData();
	std::cout <<  std::format(R"(
SytemCpuTotal: {:.2f}%
ProcessCpuTotal: {:.2f}%
ProcessUserMemory: {:.2f}MB
ProcessNonPaged: {:.2f}MB
SystemAvailableMemory: {:.2f}GB
SystemNonPaged: {:.2f}MB                  
OutDataSize: {}KB
InDataSize: {}KB
)",GetSystemCpuTotal(),GetProcessCpuTotal(),GetProcessUserMemoryByMB(),GetProcessNonPagedByMB(),GetSystemAvailMemoryByGB(),GetSystemNonPagedByMB(),GetOutDataSizeByKB(),GetInDataSizeByKB());
}

void PerformanceMonitor::AddInterface(std::string interfaceIp)
{
	//if (interfaceIp == "127.0.0.1")
	//{
	//	_interfaceIndexSet.insert(1);
	//	return;
	//}
	PIP_ADAPTER_INFO pAdapterInfo;
	PIP_ADAPTER_INFO pAdapter = NULL;
	DWORD dwRetVal = 0;
	UINT i;

	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
	pAdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
	if (pAdapterInfo == NULL) {
		printf("Error allocating memory needed to call GetAdaptersinfo\n");
		return ;
	}
	// Make an initial call to GetAdaptersInfo to get
	// the necessary size into the ulOutBufLen variable
	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
		free(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
		if (pAdapterInfo == NULL) {
			printf("Error allocating memory needed to call GetAdaptersinfo\n");
			return ;
		}
	}

	if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
		
		pAdapter = pAdapterInfo;
		while (pAdapter) {
			if (interfaceIp == "0.0.0.0")
			{
				_interfaceIndexSet.insert(pAdapter->Index);
			}
			else
			{
				if (strcmp(pAdapter->IpAddressList.IpAddress.String, interfaceIp.data()) == 0)
				{
					_interfaceIndexSet.insert(pAdapter->Index);
					break;
				}
			}
			pAdapter = pAdapter->Next;
		}
	}
	else {
		printf("GetAdaptersInfo failed with error: %d\n", dwRetVal);
	}
	if (pAdapterInfo)
		free(pAdapterInfo);
}
