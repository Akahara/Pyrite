#pragma once

#include "editor/EditorActor.h"
#include <unordered_map>
#include "scene/RenderableActorCollection.h"

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
		void Init(const pyr::RegisteredRenderableActorCollection& sceneActors); // Used as a scene init for now, needs to be plugged somewhere
		void Shutdown();

		std::unordered_map<int, EditorActor*> RegisteredActors;

	};



}