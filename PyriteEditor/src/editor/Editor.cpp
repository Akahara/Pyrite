#include "Editor.h"

#include "editor/bridges/pf_StaticMesh.h"
#include "editor/bridges/Lights/pf_Light.h"
#include "editor/bridges/pf_BillboardHUD.h"

#include "world/Actor.h"
#include "world/Mesh/StaticMesh.h"
#include <scene/scene.h>
#include <scene/SceneManager.h>

pye::Editor::Editor()
{
	assets.lightbulb = m_editorAssetsLoader.loadTexture(L"editor/icons/world/lights/lightbulb.png", false);
	assets.spotlight = m_editorAssetsLoader.loadTexture(L"editor/icons/world/lights/spotlight.png", false);
	assets.directionalLight = m_editorAssetsLoader.loadTexture(L"editor/icons/world/lights/directionalLight.png", false);
}

// probably a terrible way of handling this lol
void pye::Editor::UpdateRegisteredActors(const pyr::RegisteredRenderableActorCollection& sceneActors)
{
	ClearRegisteredActors();

	for (const pyr::StaticMesh* staticMesh : sceneActors.meshes)
	{
		pye::pf_StaticMesh* editorSMesh = new pye::pf_StaticMesh;
		editorSMesh->sourceMesh = (pyr::StaticMesh*)staticMesh;
		RegisteredActors[staticMesh->GetActorID()] = editorSMesh;
	}

	for (pyr::BaseLight* light: sceneActors.lights.toBaseLights())
	{
		pye::pf_Light* editorLight = new pye::pf_Light;
		editorLight->sourceLight = (pyr::BaseLight*)light;

		pyr::Billboard* bb = new pyr::Billboard;
		bb->type = pyr::Billboard::HUD;

		switch (light->getType())
		{
		case pyr::LightTypeID::Directional	: bb->texture = &assets.directionalLight; break;
		case pyr::LightTypeID::Point			: bb->texture = &assets.lightbulb; break;
		case pyr::LightTypeID::Spotlight		: bb->texture = &assets.spotlight; break;
		default: break;
		}

		pf_BillboardHUD* editorBillboard = new pf_BillboardHUD;
		editorBillboard->editorBillboard = bb;
		editorBillboard->coreActor = light;
		bb->transform.position = { light->GetTransform().position.x,light->GetTransform().position.y,light->GetTransform().position.z };
		WorldHUD.push_back(editorBillboard);
		RegisteredActors[editorBillboard->editorBillboard->GetActorID()] = editorBillboard;

	}
}

void pye::Editor::ClearRegisteredActors()
{
	for (auto* editBillboard : WorldHUD)
	{
		delete editBillboard->editorBillboard;
		delete editBillboard;
	}
	for (auto& [id, actor] : RegisteredActors)
	{
		RegisteredActors[id] = nullptr;
		if (actor)
			delete actor;
	}
	WorldHUD.clear();
	RegisteredActors.clear();
}

void pye::Editor::UnselectedAllActors()
{
	pyr::Scene* currentScene = pyr::SceneManager::getActiveScene();
	if (!PYR_ENSURE(currentScene))
	{
		PYR_LOG(LogEditor, WARN, "Trying to unselected all actors when no scene is active !");
		return;
	}

	// TODO : note all scene have render graphs lol, how do we handle this
	//currentScene->



}
