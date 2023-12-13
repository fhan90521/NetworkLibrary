
#include "CacheTracer.h"
#include <vector>
#include <algorithm>
bool CacheTracer::trace(void* variable)
{
	ADDRESS address = (ADDRESS)variable;
	int index =((address & MASK) >> 6);
	TAG tag = address >> 12;
	tagMap[tag]++;
	traceCnt++;
	//cout << index << endl;
	bool isHit = false;
	for (int i = 0; i < way_num; i++)
	{
		cache_counter[index][i]++;
		//cout << "tag " << std::hex << tag << endl;
		//cout << std::hex << cache[index][i] << endl;
		if (cache[index][i] == tag)
		{
			hit++;
			
			isHit = true;
			cache_counter[index][i] = 0;
		}
	}
	if (isHit == false)
	{
		miss++;
		int changed_waynum=0;
		int top_count = 0;
		for (int i = 0; i < way_num; i++)
		{
			if (top_count < cache_counter[index][i])
			{
				top_count = cache_counter[index][i];
				changed_waynum=i;
			}
		}
		cache[index][changed_waynum] = tag;
	}

	return isHit;
}
void CacheTracer::trace(void* variable, string name)
{
	
	if (trace(variable) == false)
	{
		nameMissMap[name]++;
	}
}

void CacheTracer::show_hit_miss()
{
	cout << "hit: " << hit << " " << "miss: " << miss << endl;
}
void CacheTracer::show_miss_name()
{
	for (auto& pairNameMiss : nameMissMap)
	{
		cout << "이름 : " << pairNameMiss.first << "miss 횟수 : " << pairNameMiss.second << '\n';
	}
}
void CacheTracer::ShowTagNumTraceNum()
{
	cout << "할당 받은 tag수 : " << tagMap.size() << " 할당 받은 횟수: " << traceCnt << '\n';
}
