#pragma once
#include <memory>
#include <functional>
using CallbackType = std::function<void()>;
class CJob
{
public:
	CJob(CallbackType&& callback) : _callback(std::move(callback))
	{
	}
	template<typename T, typename Ret, typename... Args>
	CJob(T* owner, Ret(T::* memFunc)(Args...), Args&&... args)
	{
		_callback = [owner, memFunc, args...]()
		{
			(owner->*memFunc)(args...);
		};
	}
	void Execute()
	{
		_callback();
	}
private:
	CallbackType _callback;
};