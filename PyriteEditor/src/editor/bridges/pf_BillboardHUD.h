#pragma once

#include "editor/EditorActor.h"
#include "world/Billboards/Billboard.h"
#include <imgui.h>

namespace pye
{
	template<>
	class EditorActor_Impl<pyr::Billboard> : public EditorActor
	{
	public:
		pyr::Billboard* editorBillboard;
		pyr::Actor* coreActor;

	public:

		virtual void inspect() override
		{
		}
	};

	using pf_BillboardHUD = EditorActor_Impl<pyr::Billboard>;
}