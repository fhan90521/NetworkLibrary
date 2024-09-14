#pragma once
#define TAG _int64
#define ADDRESS _int64
#define CACHEINDEX 64
#define MASK 0x00000FFF
#include <iostream>
#include <map>
#include <set>
class CacheTracer
{

private:
	
	int way_num;
	int hit = 0;
	int miss = 0;
	int traceCnt = 0;
	TAG* cache[CACHEINDEX];
	int* cache_counter[CACHEINDEX];
	std::map<std::string, int > nameMissMap;
	std::map<TAG,int> tagMap;
	bool trace(void* variable);
public:
	CacheTracer(int _way_num)
	{
		way_num = _way_num;
		for (int i = 0; i < CACHEINDEX; i++)
		{
			cache[i] = new TAG[way_num];
			cache_counter[i] = new int[way_num]();
		}
	};
	~CacheTracer()
	{
		for (int i = 0; i < CACHEINDEX; i++)
		{
			delete[] cache[i];
		}
	}
	void clear()
	{
		hit = 0;
		miss = 0;
		nameMissMap.clear();
	}
	void trace(void* variable, std::string _name);
	void show_hit_miss();
	void show_miss_name();
	void ShowTagNumTraceNum();
};