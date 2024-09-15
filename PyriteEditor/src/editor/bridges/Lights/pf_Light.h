#pragma once

#include "world/Lights/Light.h"
#include "utils/debug.h"

namespace pye
{
	// pyr format

	struct pf_Light
	{
		
		// -- // 
		pyr::BaseLight* sourceLight;

	public:
		enum Type { POINT, DIR, SPOT, NONE };
		// -- //
		std::string name;
		Type type;
	};

	struct pf_LightsCollection
	{
		pf_LightsCollection() = default;
		pf_LightsCollection(pyr::LightsCollections* source)
		{
			PYR_ASSERT(source);
			sourceCollection = source;
			UpdateEditorFormat();
		}
		// -- // 
		pyr::LightsCollections* sourceCollection = nullptr;


		// -- // 
		std::vector<pf_Light> EditorFormat;
		int selectedId = 0;
		pf_Light* selectedLight = nullptr;

	public:

		void UpdateEditorFormat()
		{
			EditorFormat.clear();
			int id = 1;
			for (pyr::PointLight& pl : sourceCollection->Points)
				EditorFormat.push_back(pf_Light{ .sourceLight = &pl, .name = std::format("Pointlight #{}", id++), .type = pf_Light::POINT});

			id = 1;
			for (pyr::SpotLight& sl : sourceCollection->Spots)
				EditorFormat.push_back(pf_Light{ .sourceLight = &sl, .name = std::format("SpotLight #{}", id++), .type = pf_Light::SPOT });

			id = 1;
			for (pyr::DirectionalLight& dl : sourceCollection->Directionals)
				EditorFormat.push_back(pf_Light{ .sourceLight = &dl, .name = std::format("DirectionalLight #{}", id++), .type = pf_Light::DIR });

		}
	};

}