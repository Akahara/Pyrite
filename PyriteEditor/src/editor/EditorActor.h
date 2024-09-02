#pragma once

#include <cstdint>

namespace pye
{

	class EditorActor
	{
		// Don't give a reference to Actor class because the implementation will handle it.
	public:
		virtual void inspect() {};
		virtual ~EditorActor() = default;
	};

	template<class T>
	class EditorActor_Impl;
}