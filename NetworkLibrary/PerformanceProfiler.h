#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
#include "MyWindow.h"
#include "GetMyThreadID.h"
class PerformanceProfiler
{
private:
	enum
	{
		MAX_SAMPLE_CNT = 32,
		THREAD_CNT = 16
	};
	struct PROFILE_SAMPLE
	{
		LARGE_INTEGER start_time;
		_int64 totalTime = 0;
		_int64 Min[2]{ INT_MAX,INT_MAX };
		_int64 Max[2]{};
		_int64 callCnt = 0;
		string name;
		bool bUsed = false;
		void Initial()
		{
			start_time.QuadPart = 0;
			totalTime = 0;
			Min[0] = INT_MAX;
			Min[1] = INT_MAX;
			Max[0] = 0;
			Max[1] = 0;
			callCnt = 0;
			bUsed = true;
		}
		void SetMax(_int64 val)
		{
			if (Max[0] < val)
			{
				Max[0] = val;
			}
			else if (Max[1] < val)
			{
				Max[1] = val;
			}
		}
		void SetMin(_int64 val)
		{
			if (Min[0] > val)
			{
				Min[0] = val;
			}
			else if (Min[1] > val)
			{
				Min[1] = val;
			}
		}
	};
	static PerformanceProfiler _instance;
	PROFILE_SAMPLE sampleArr[THREAD_CNT][MAX_SAMPLE_CNT];
	PerformanceProfiler() {};
	~PerformanceProfiler() {};
public:
	static PerformanceProfiler* GetInstance()
	{
		return &_instance;
	}
	void SetName(int index, string name)
	{
		sampleArr[GetMyThreadID()][index].name = name;

	}
	void ProfileReset(const int& index)
	{
		for (int i = 0; i < THREAD_CNT; i++)
		{
			sampleArr[i][index].bUsed = false;
		}
	}
	void ProfileBegin(const int& index)
	{
		PROFILE_SAMPLE& sample = sampleArr[GetMyThreadID()][index];
		if (sample.start_time.QuadPart != 0 || sample.name.empty())
		{
			cout << "end호출안하거나 name설정 안함" << endl;
			DebugBreak();
		}
		if (sample.bUsed == false)
		{
			sample.Initial();
		}
		QueryPerformanceCounter(&sample.start_time);
	}
	void ProfileEnd(const int& index)
	{
		PROFILE_SAMPLE& sample = sampleArr[GetMyThreadID()][index];
		if (sample.start_time.QuadPart == 0)
		{
			cout << "start호출안함" << endl;
			DebugBreak();
		}
		LARGE_INTEGER end_time;
		QueryPerformanceCounter(&end_time);
		_int64 time_diff = end_time.QuadPart - sample.start_time.QuadPart;
		sample.callCnt++;
		sample.totalTime += time_diff;
		sample.SetMax(time_diff);
		sample.SetMin(time_diff);
		sample.start_time.QuadPart = 0;
	}
	void ProfileDataOutText(const string file_name)
	{
		ofstream fout(file_name);
		if (!fout)
		{
			cout << "프로파일러 파일 출력 에러" << endl;
			DebugBreak();
		}
		fout << setw(20) << "Tag" << " | " << setw(20) << "Average" << " | " << setw(20) << "Min" << " | " << setw(20) << "Max" << " | " << setw(20) << "Call" << " | " << endl;
		fout << "----------------------------------------------------------------------------------------------------------------------------------" << endl;
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);

		for (int i = 0; i < MAX_SAMPLE_CNT; i++)
		{
			PROFILE_SAMPLE totalSample;
			for (int threadID = 0; threadID < THREAD_CNT; threadID++)
			{
				PROFILE_SAMPLE& sample = sampleArr[threadID][i];
				if (sample.bUsed == false)
				{
					continue;
				}
				totalSample.totalTime += sample.totalTime - sample.Max[0] - sample.Max[1] - sample.Min[0] - sample.Min[1];
				totalSample.callCnt += sample.callCnt;
				totalSample.name = sample.name;
				totalSample.SetMax(sample.Max[0]);
				totalSample.SetMax(sample.Max[1]);
				totalSample.SetMin(sample.Min[0]);
				totalSample.SetMin(sample.Min[1]);
			}
			if (!totalSample.name.empty())
			{
				double total_micro = totalSample.totalTime / (double)frequency.QuadPart * 1000000;
				fout.precision(4);
				fout << fixed << setw(20) << totalSample.name << " | "
					<< setw(20) << total_micro / (totalSample.callCnt - 4) << " | "
					<< setw(20) << totalSample.Min[0] / (double)frequency.QuadPart * 1000000 << " | "
					<< setw(20) << totalSample.Max[0] / (double)frequency.QuadPart * 1000000 << " | "
					<< setw(20) << totalSample.callCnt << " | " << endl;
			}
		}
	}
};
PerformanceProfiler PerformanceProfiler::_instance;
