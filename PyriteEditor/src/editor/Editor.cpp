#include "Editor.h"

#include "editor/bridges/pf_StaticMesh.h"

#include "world/Actor.h"
#include "world/Mesh/StaticMesh.h"

void pye::Editor::Init(const pyr::RegisteredRenderableActorCollection& sceneActors)
{
	for (const pyr::StaticMesh* staticMesh : sceneActors.meshes)
	{
		pye::pf_StaticMesh* editorSMesh = new pye::pf_StaticMesh;
		editorSMesh->sourceMesh = (pyr::StaticMesh*)staticMesh;
		RegisteredActors[staticMesh->GetActorID()] = editorSMesh;
	}
}

void pye::Editor::Shutdown()
{
	for (auto& [id, actor] : RegisteredActors)
	{
		delete actor;
	}
}
