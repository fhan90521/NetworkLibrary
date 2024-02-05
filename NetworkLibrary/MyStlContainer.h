#include "PoolAllocatorForSTL.h"
#include <vector>
#include <list>
#include <memory>
#include <queue>
#include <stack>
#include <map>
#include <set>
#include<string>
#include<array>
#include <unordered_map>
#include <unordered_set>
using namespace std;

template<typename Type,typename size_t Len>
using Array = array<Type, Len>;

template<typename Type>
using Vector = vector<Type, PoolAllocatorForSTL<Type>>;

template<typename Type>
using List = list<Type, PoolAllocatorForSTL<Type>>;

template<typename Key, typename Type, typename Pred = less<Key>>
using Map = map<Key, Type, Pred, PoolAllocatorForSTL<pair<const Key, Type>>>;

template<typename Key, typename Pred = less<Key>>
using Set = set<Key, Pred, PoolAllocatorForSTL<Key>>;

template<typename Type>
using Deque = deque<Type, PoolAllocatorForSTL<Type>>;

template<typename Type, typename Container = Deque<Type>>
using Queue = queue<Type, Container>;

template<typename Type, typename Container = Deque<Type>>
using Stack = stack<Type, Container>;

template<typename Type, typename Container = Vector<Type>, typename Pred = less<typename Container::value_type>>
using PriorityQueue = priority_queue<Type, Container, Pred>;

using String = basic_string<char, char_traits<char>, PoolAllocatorForSTL<char>>;

using WString = basic_string<wchar_t, char_traits<wchar_t>, PoolAllocatorForSTL<wchar_t>>;

template<typename Key, typename Type, typename Hasher = hash<Key>, typename KeyEq = equal_to<Key>>
using HashMap = unordered_map<Key, Type, Hasher, KeyEq, PoolAllocatorForSTL<pair<const Key, Type>>>;

template<typename Key, typename Hasher = hash<Key>, typename KeyEq = equal_to<Key>>
using HashSet = unordered_set<Key, Hasher, KeyEq, PoolAllocatorForSTL<Key>>;