#include <iostream>
#include <string>
#include <vector>
#include <ranges>
#include <algorithm>
#include <functional>

template<class R, class ...Args>
using Lambda = std::function<R(Args...)>;

template<class R, class ...Args>
using FunctionPtr = R(*)(Args...);

template<class T, class R, class ...Args>
using MemberFunctionPtr = R(T::*)(Args...);

using DelegateHandle = uint32_t;
inline DelegateHandle nextId = 0;

template<class ...FwdArgs>
class Delegate
{

public:

	Delegate() = default;
	~Delegate() { RemoveAll(); }

	Delegate(Delegate<FwdArgs...>&& moved)
		: callbacks{ std::exchange(moved.callbacks, {}) }
		, debugName{ std::exchange(moved.debugName, {}) }
	{}


	std::string debugName;

	void NotifyAll(FwdArgs... args) const
	{
		for (auto& [handle, callback] : callbacks)
			callback->invoke(args...);
	}

	// For some reasons this definition is not needed, and function ptrs would work correctly with auto&& ? 
	// I keep it as it makes things a bit clearer and there are things i overlook
	template<class F>
	DelegateHandle BindCallback(F* functionptr)
	{
		Callback* callback = new FunctionPtrCallback{ functionptr };
		DelegateHandle handle = nextId++;
		callbacks[handle] = callback;
		return handle;
	}

	DelegateHandle BindCallback(auto&& lambda)
	{
		Callback* callback = new LambdaCallback{ lambda };
		DelegateHandle handle = nextId++;
		callbacks[handle] = callback;
		return handle;
	}
	template<class O>
	DelegateHandle BindCallback(O& object, auto&& fnPtr)
	{
		Callback* callback = new MemberFunctionPtrCallback{ object, fnPtr };
		DelegateHandle handle = nextId++;
		callbacks[handle] = callback;
		return handle;
	}

	void RemoveCallback(DelegateHandle handle)
	{
		delete callbacks[handle];
	}

	void RemoveAll()
	{
		for (auto& [handle, callback] : callbacks)
			RemoveCallback(handle);
		callbacks.clear();
	}


private:

	struct Callback {
		virtual void invoke(FwdArgs... args) = 0;
	};

	std::unordered_map<DelegateHandle, Callback*> callbacks;


	template<class L>	
	struct LambdaCallback : Callback
	{
		static_assert(std::invoke_result<L, FwdArgs...>::_Is_invocable::value, 
			"Specified lambda callback takes in parameters that do not match what the event will dispatch !");
		
		using return_type_t = std::invoke_result<L, FwdArgs...>::type;
		Lambda<return_type_t, FwdArgs...> lambda;
		
		LambdaCallback(L fn)
		{
			lambda = fn;
		}
		virtual void invoke(FwdArgs... args) override
		{
			lambda(args...);
		};

	};

	template<class FnPtr>
	struct FunctionPtrCallback : Callback
	{
		FunctionPtr<typename std::invoke_result<FnPtr, FwdArgs...>::type, FwdArgs...> ptr;
		FunctionPtrCallback(FnPtr fptr)
		{
			ptr = fptr;
		}
		virtual void invoke(FwdArgs... args) override
		{
			ptr(args...);
		};
	};

	template<class O, class FnPtr>
	struct MemberFunctionPtrCallback : Callback
	{
		O& object;
		MemberFunctionPtr<O, typename std::invoke_result<FnPtr, O, FwdArgs...>::type, FwdArgs...> ptr;
		MemberFunctionPtrCallback(O& ref, FnPtr fptr)
			: object{ ref }
		{
			ptr = fptr;
		}
		virtual void invoke(FwdArgs... args) override
		{
			std::invoke(ptr, object, args...);
		};
	};

};
