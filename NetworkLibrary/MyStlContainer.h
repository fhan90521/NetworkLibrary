#include "CommonPoolAllocatorForStd.h"
#include <list>
#include <unordered_map>
#include <vector>
using namespace std;

template<typename Type>
using List = list<Type, CommonPoolAllocatorForStd<Type>>;

template<typename Type>
using Vector = vector<Type, CommonPoolAllocatorForStd<Type>>;

template<typename Key, typename Type, typename Hasher = hash<Key>, typename KeyEq = equal_to<Key>>
using UnorderedMap = unordered_map<Key, Type, Hasher, KeyEq, CommonPoolAllocatorForStd<pair<const Key, Type>>>;