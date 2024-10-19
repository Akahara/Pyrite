#pragma once

#include <vector>

#include "world/Lights/Light.h"

/// RenderableActorCollection
/// 
/// Contains multiple types of actors that can be renderered.
/// This collection is stored in the scene, where you register every single actor that participates somehow, and should be used by Views later on (for culling and stuff)

namespace pyr
{
	struct RegisteredRenderableActorCollection
	{
		std::vector<const class StaticMesh*> meshes;
		std::vector<const struct Billboard*> billboards;
		pyr::LightsCollections lights;

		void registerForFrame(const StaticMesh* mesh)		{ meshes.push_back(mesh); }
		void registerForFrame(const Billboard* billboard)	{ billboards.push_back(billboard); }
		//void registerForFrame(const BaseLight* light)		{ lights.AddLight(light); }

		void clear() { meshes.clear(); billboards.clear(); }
	};
}