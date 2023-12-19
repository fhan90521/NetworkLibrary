#include "CommonPoolAllocatorForSTL.h"
#include <list>
#include <unordered_map>
#include <vector>
using namespace std;

template<typename Type>
using List = list<Type, CommonPoolAllocatorForSTL<Type>>;

template<typename Type>
using Vector = vector<Type, CommonPoolAllocatorForSTL<Type>>;

template<typename Key, typename Type, typename Hasher = hash<Key>, typename KeyEq = equal_to<Key>>
using UnorderedMap = unordered_map<Key, Type, Hasher, KeyEq, CommonPoolAllocatorForSTL<pair<const Key, Type>>>;