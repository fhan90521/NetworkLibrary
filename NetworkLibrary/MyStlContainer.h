#include "LockPoolAllocatorForSTL.h"
#include <list>
#include <unordered_map>
#include <vector>
using namespace std;

template<typename Type>
using List = list<Type, LockPoolAllocatorForSTL<Type>>;

template<typename Type>
using Vector = vector<Type, LockPoolAllocatorForSTL<Type>>;

template<typename Key, typename Type, typename Hasher = hash<Key>, typename KeyEq = equal_to<Key>>
using UnorderedMap = unordered_map<Key, Type, Hasher, KeyEq, LockPoolAllocatorForSTL<pair<const Key, Type>>>;