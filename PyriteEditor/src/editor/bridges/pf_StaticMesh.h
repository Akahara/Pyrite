#pragma once

#include "editor/EditorActor.h"

namespace pyr 
{
	class StaticMesh;
}

namespace pye
{
	template<>
	class EditorActor_Impl<pyr::StaticMesh> : public EditorActor
	{
	public:
		pyr::StaticMesh* sourceMesh;

	public:

		virtual void inspect() override
		{
		
		
		}
	};



}