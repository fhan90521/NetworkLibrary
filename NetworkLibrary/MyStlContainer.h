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




template<typename T>
using  SharedPtr = std::shared_ptr<T>;

template<typename Type,typename size_t Len>
using Array = std::array<Type, Len>;

template<typename Type>
using Vector = std::vector<Type, PoolAllocatorForSTL<Type>>;

template<typename Type>
using List = std::list<Type, PoolAllocatorForSTL<Type>>;

template<typename Key, typename Type, typename Pred = std::less<Key>>
using Map = std::map<Key, Type, Pred, PoolAllocatorForSTL<std::pair<const Key, Type>>>;

template<typename Key, typename Pred = std::less<Key>>
using Set = std::set<Key, Pred, PoolAllocatorForSTL<Key>>;

template<typename Type>
using Deque = std::deque<Type, PoolAllocatorForSTL<Type>>;

template<typename Type, typename Container = Deque<Type>>
using Queue = std::queue<Type, Container>;

template<typename Type, typename Container = Deque<Type>>
using Stack = std::stack<Type, Container>;

template<typename Type, typename Container = Vector<Type>, typename Pred = std::less<typename Container::value_type>>
using PriorityQueue = std::priority_queue<Type, Container, Pred>;

using String = std::basic_string<char, std::char_traits<char>, PoolAllocatorForSTL<char>>;

using WString = std::basic_string<wchar_t, std::char_traits<wchar_t>, PoolAllocatorForSTL<wchar_t>>;

template<typename Key, typename Type, typename Hasher = std::hash<Key>, typename KeyEq = std::equal_to<Key>>
using HashMap = std::unordered_map<Key, Type, Hasher, KeyEq, PoolAllocatorForSTL<std::pair<const Key, Type>>>;

template<typename Key, typename Hasher = std::hash<Key>, typename KeyEq = std::equal_to<Key>>
using HashSet = std::unordered_set<Key, Hasher, KeyEq, PoolAllocatorForSTL<Key>>;