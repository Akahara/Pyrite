#pragma once

#include "editor/EditorActor.h"
#include "editor/bridges/pf_BillboardHUD.h"
#include "world/Billboards/Billboard.h"
#include <unordered_map>
#include "scene/RenderableActorCollection.h"
#include "utils/debug.h"

static inline PYR_DEFINELOG(LogEditor, VERBOSE);

namespace pye
{
	class Editor
	{
	public:

		static Editor& Get()
		{
			static Editor gEditor;
			return gEditor;
		}
		// Call this once ?


		// Used as a scene init for now, needs to be plugged somewhere
		void UpdateRegisteredActors(const pyr::RegisteredRenderableActorCollection& sceneActors); 
		void ClearRegisteredActors();

		std::unordered_map<int, EditorActor*> RegisteredActors;
		std::vector<const pye::pf_BillboardHUD*> WorldHUD;
		pyr::GraphicalResourceRegistry m_editorAssetsLoader;

		struct EditorAssets
		{
			pyr::Texture lightbulb;

		} assets;

	public:

		void UnselectedAllActors();

	private:

		Editor();


	};
}