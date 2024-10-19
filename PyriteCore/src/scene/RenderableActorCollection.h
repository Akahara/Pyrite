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
	};
}