#pragma once
#include <memory>
#include <functional>
using CallbackType = std::function<void()>;
class Job
{
public:
	virtual ~Job() {};
	Job(CallbackType&& callback) : _callback(std::move(callback))
	{
	}
	template<typename T, typename Ret, typename... Params, typename... Args>
	Job(T* owner, Ret(T::* memFunc)(Params...), Args&&... args)
	{
		_callback = [owner, memFunc, ...args = std::forward<Args>(args)]()mutable
		{
			(owner->*memFunc)(std::forward<Args>(args)...);
		};
	}
	void Execute()
	{
		_callback();
	}
private:
	CallbackType _callback;
};