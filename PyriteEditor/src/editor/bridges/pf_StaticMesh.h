#pragma once

#include "editor/EditorActor.h"
#include "world/Mesh/StaticMesh.h"
#include <imgui.h>

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
			ImGui::Begin("Inspection Window");
			ImGui::DragFloat3("Position", &sourceMesh->getTransform().position.x);
			ImGui::End();
		}
	};

	using pf_StaticMesh = EditorActor_Impl<pyr::StaticMesh>;
}