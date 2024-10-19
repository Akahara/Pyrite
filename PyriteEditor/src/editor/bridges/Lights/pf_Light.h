#pragma once

#include "world/Lights/Light.h"
#include "utils/debug.h"

namespace pyr
{
	struct LightsCollections;
	struct BaseLight;
	enum LightTypeID : uint32_t;
}

namespace pye
{
	template<>
	class EditorActor_Impl<pyr::BaseLight> : public EditorActor
	{
	public:
		pyr::BaseLight* sourceLight;

	public:
		std::string name;
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
		std::vector<pf_Light> EditorFormatLights;
		int selectedId = 0;
		pf_Light* selectedLight = nullptr;

		bool bIsWidgetDirty = false; // Force recomputation of scene actors, should not be here (should have some kind of event system)

	public:

		void registerLight(pyr::BaseLight* l) // stupid signature, todo make all of this variants
		{
			pf_Light editorLight; // todo remove this stupid heap allocated stuff
			editorLight.sourceLight = l;
			pyr::LightTypeID lightType = l->getType();
			size_t id = std::ranges::count_if(EditorFormatLights, [lightType](const pf_Light& light) { return light.sourceLight->getType() == lightType; });
			switch (lightType)
			{
				case pyr::LightTypeID::Point: editorLight.name = std::format("Pointlight #{}", id + 1); break;
				case pyr::LightTypeID::Directional: editorLight.name = std::format("DirectionalLight #{}", id++); break;
				case pyr::LightTypeID::Spotlight: editorLight.name = std::format("SpotLight #{}", id++); break;
			}
			EditorFormatLights.push_back(editorLight);
		}

		void removeLight(const pye::pf_Light& editorLight) 
		{
			auto it = std::find_if(EditorFormatLights.begin(), EditorFormatLights.end(), [editorLight](const pf_Light& l) { return l.sourceLight == editorLight.sourceLight; });
			if (it != EditorFormatLights.end()) {
				EditorFormatLights.erase(it);
			}
		}
	};

}
