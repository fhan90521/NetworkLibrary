#pragma once
#include "Windows.h"
#include <map>
#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
using namespace std;

class PerformanceProfiler
{
private:
	struct PROFILE_SAMPLE
	{
		LARGE_INTEGER start_time;
		_int64 iTotalTime=0;
		_int64 iMin[2]{INT_MAX,INT_MAX};
		_int64 iMax[2]{};
		_int64 iCall=0;
	};
	inline static PerformanceProfiler* _instance;
	PerformanceProfiler() {};
	~PerformanceProfiler() {};
	map<string, PROFILE_SAMPLE> sample_map;
public:
	static PerformanceProfiler* GetInstance()
	{
		if (_instance == nullptr)
		{
			_instance = new PerformanceProfiler;
		}
		return _instance;
	}
	void ProfileBegin(const string& key_tag)
	{
		PROFILE_SAMPLE& sample = sample_map[key_tag];
		if (sample.start_time.QuadPart != 0)
		{
			cout << "end호출안함" << endl;
			DebugBreak();
		}
		QueryPerformanceCounter(&sample.start_time);
	}
	void ProfileEnd(const string& key_tag)
	{
		PROFILE_SAMPLE& sample = sample_map[key_tag];
		if (sample.start_time.QuadPart == 0)
		{
			cout << "start호출안함" << endl;
			DebugBreak();
		}
		LARGE_INTEGER end_time;
		QueryPerformanceCounter(&end_time);
		_int64 time_diff = end_time.QuadPart - sample.start_time.QuadPart;
		sample.iCall++;
		sample.iTotalTime += time_diff;
		if (sample.iMax[0] < time_diff)
		{
			sample.iMax[0] = time_diff;
		}
		else if (sample.iMax[1] < time_diff)
		{
			sample.iMax[1] = time_diff;
		}
		
		if (sample.iMin[0] > time_diff)
		{
			sample.iMin[0] = time_diff;
		}
		else if (sample.iMin[1] > time_diff)
		{
			sample.iMin[1] = time_diff;
		}
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
		fout << setw(20) << "Tag" <<" | " << setw(20) << "Average" << " | " << setw(20) << "Min" << " | " << setw(20) << "Max" << " | " << setw(20) << "Call" << " | " << endl;
		fout << "----------------------------------------------------------------------------------------------------------------------------------" << endl;
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		
		for (const auto& iter : sample_map)
		{
			_int64 total_time = iter.second.iTotalTime - iter.second.iMax[0] - iter.second.iMax[1] - iter.second.iMin[0] - iter.second.iMin[1];
			double total_micro = total_time /(double)frequency.QuadPart * 1000000;
			fout.precision(4);
			fout<<fixed << setw(20) << iter.first << " | " 
				<< setw(20) << total_micro/(iter.second.iCall-4) << " | "
				<< setw(20) << iter.second.iMin[0] / (double)frequency.QuadPart * 1000000 << " | "
				<< setw(20) << iter.second.iMax[0] / (double)frequency.QuadPart * 1000000 << " | "
				<< setw(20) << iter.second.iCall << " | " << endl;
		}
		fout.close();
	}
	void ProfileReset()
	{
		sample_map.clear();
	}

};