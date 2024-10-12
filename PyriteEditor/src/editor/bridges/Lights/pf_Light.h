#pragma once

#include "world/Lights/Light.h"
#include "utils/debug.h"

namespace pye
{
	template<>
	class EditorActor_Impl<pyr::BaseLight> : public EditorActor
	{
	public:
		pyr::BaseLight* sourceLight;

	public:
		enum Type { UNDEFINED, DIR, POINT, SPOT,};
		std::string name;
		Type type;
	};

	using pf_Light = EditorActor_Impl<pyr::BaseLight>;

	struct pf_LightsCollection
	{
		pf_LightsCollection() = default;
		pf_LightsCollection(pyr::LightsCollections* source)
		{
			PYR_ASSERT(source);
			sourceCollection = source;
		}
		// -- // 
		pyr::LightsCollections* sourceCollection = nullptr; // this is the currentScene collection
		
		// -- Used by the widget to modify the engine-side light in the sourceCollection // 
		std::vector<pf_Light*> EditorFormatPtrs;
		int selectedId = 0;
		pf_Light* selectedLight = nullptr;

		bool bIsWidgetDirty = false; // Force recomputation of scene actors

	public:

		void registerLight(pyr::BaseLight* l) // stupid signature, todo make all of this variants
		{
			pf_Light* editorLight = new pf_Light;
			pf_Light::Type type = (pf_Light::Type)l->getType();
			editorLight->sourceLight = l;
			size_t id = std::ranges::count_if(EditorFormatPtrs, [type](pf_Light* light) { return light->type == type; });
			switch (type)
			{
				case pf_Light::POINT: editorLight->name = std::format("Pointlight #{}", id + 1); break;
				case pf_Light::SPOT : editorLight->name = std::format("SpotLight #{}", id++); break;
				case pf_Light::DIR : editorLight->name = std::format("DirectionalLight #{}", id++); break;
			}
			editorLight->type = type;
			EditorFormatPtrs.push_back(editorLight);
		}

		void removeLight(pye::pf_Light* editorLight) 
		{
			auto it = std::find(EditorFormatPtrs.begin(), EditorFormatPtrs.end(),editorLight);
			if (it != EditorFormatPtrs.end()) {
				EditorFormatPtrs.erase(it);
			}
			delete *it;
		}
		
	
		
		// todo inspect, steall the wcode in the widget or smth
	};

}