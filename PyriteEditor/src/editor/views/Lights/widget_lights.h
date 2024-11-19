#pragma once

#include "imgui.h"

#include "display/texture.h"

#include "editor/bridges/Lights/pf_Light.h"
#include "editor/views/widget.h"
#include "editor/Editor.h"
#include "editor/EditorEvents.h"


/// <summary>
/// Fatass chunk of code that basically fetches the current scene lights (using pyr::LightCollection)
/// and register all of the engine-side lights as editor-side ones (they all have a pointer to the base engine light)
///
/// Then we inspect each editor light, with adding/deleting operations ect! 
/// </summary>

namespace pye
{
	namespace widgets
	{

		struct LightCollectionWidget : public Widget 
		{

		private:

			pye::pf_LightsCollection LightsCollectionView; // fetched from registered actors, should be a scene specific widget

		public:

			virtual void display() override
			{
				// -- Ensure that we have a light collection to inspect
				pyr::Scene* currentScene = pyr::SceneManager::getActiveScene();
				if (!currentScene) return;

				if ( !LightsCollectionView.sourceCollection ||
					  LightsCollectionView.sourceCollection != &currentScene->SceneActors.lights ||
					LightsCollectionView.bIsWidgetDirty)
				{
					// Initial update, register all lights
	
					LightsCollectionView.sourceCollection = &currentScene->SceneActors.lights;
					auto baseLights = LightsCollectionView.sourceCollection->toBaseLights();
					LightsCollectionView.EditorFormatLights.clear();
					for (pyr::BaseLight* l : baseLights)
						LightsCollectionView.registerLight(l);

					pye::Editor::Get().UpdateRegisteredActors(currentScene->SceneActors);
					LightsCollectionView.bIsWidgetDirty = false;
				}

				// -- Ensure the validity of the view collection
				PYR_ENSURE(LightsCollectionView.sourceCollection);

				// -- Import the icons and stuff
				static pyr::Texture AddIcon = getWidgetAssetRegistry().loadTexture(L"res/editor/plus.png");
				static pyr::Texture RemoveIcon = getWidgetAssetRegistry().loadTexture(L"res/editor/retirer.png");
				auto makeSelectable = [](const char* label, int id, pf_Light& ref, pf_LightsCollection &LightsCollectionView)
				{
						ImGui::PushID(label);
						if (ImGui::Selectable(label, LightsCollectionView.selectedId == id))
						{
							LightsCollectionView.selectedId = id;
							LightsCollectionView.selectedLight = &ref;

							// TODO send info to PickerPass
							pye::EditorEvents::OnActorSelectedEvent.NotifyAll(&ref);
						}
						ImGui::PopID();

				};

				// -- Start the widget
				ImGui::Begin("Lights");
				{
					// -- Behaviour of the "+" button, open a small menu popup with 3 choices
					if (ImGui::BeginPopupContextItem("AddLightPopup"))
					{
						pyr::BaseLight* bAdded = nullptr;
						if (ImGui::Selectable("Point light"))
						{
							LightsCollectionView.sourceCollection->Points.push_back({});
							LightsCollectionView.bIsWidgetDirty = true;
							LightsCollectionView.selectedLight = nullptr;
							bAdded = &LightsCollectionView.sourceCollection->Points.back();
						}
						if (ImGui::Selectable("Spot light")) 
						{
							LightsCollectionView.sourceCollection->Spots.push_back({}); 
							LightsCollectionView.bIsWidgetDirty = true;
							LightsCollectionView.selectedLight = nullptr;
							bAdded = &LightsCollectionView.sourceCollection->Spots.back();
						}
						if (ImGui::Selectable("Directional light"))
						{
							LightsCollectionView.sourceCollection->Directionals.push_back({});
							LightsCollectionView.bIsWidgetDirty = true;
							LightsCollectionView.selectedLight = nullptr;
							bAdded = &LightsCollectionView.sourceCollection->Directionals.back();
						}
						if (bAdded) pye::EditorEvents::OnActorAddedEvent.NotifyAll();
						//if (bAdded) pye::EditorEvents::OnActorAddedEvent.NotifyAll(bAdded);
						ImGui::SetNextItemWidth(-FLT_MIN);
						ImGui::EndPopup();
					}


					{
						ImGui::PushStyleColor(ImGuiCol_Button, 0);
						if (ImGui::ImageButton("AddLight", AddIcon.getRawTexture(), ImVec2{ 32,32 }))
						{
							ImGui::OpenPopup("AddLightPopup");
						}
						ImGui::SameLine();
						if (ImGui::ImageButton("RemoveLight", RemoveIcon.getRawTexture(), ImVec2{ 32,32 }))
						{
							if (LightsCollectionView.selectedLight)
							{
								PYR_LOG(LogWidgets, INFO, "Removing light.");
								LightsCollectionView.sourceCollection->RemoveLight(LightsCollectionView.selectedLight->sourceLight);
								//pye::EditorEvents::OnActorRemovedEvent.NotifyAll(LightsCollectionView.selectedLight->sourceLight);
								pye::EditorEvents::OnActorRemovedEvent.NotifyAll();
								LightsCollectionView.removeLight(*LightsCollectionView.selectedLight);

								// Try to select automatically next light
								if (LightsCollectionView.selectedId < LightsCollectionView.EditorFormatLights.size())
								{
									LightsCollectionView.selectedLight = &LightsCollectionView.EditorFormatLights[LightsCollectionView.selectedId];

								}
								else
								{
									LightsCollectionView.selectedId = 0;
									LightsCollectionView.selectedLight = nullptr;
								}

								LightsCollectionView.bIsWidgetDirty = true;

							}
						}
						ImGui::PopStyleColor();
					}
				}
				// -- Left side, as a simple list of each lights names
				ImGui::Separator();
				{
					ImGui::BeginChild("left pane", ImVec2(150, 0), ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX);
					int id = 0;
					for (pf_Light& pf_light : LightsCollectionView.EditorFormatLights)
					{
						makeSelectable(pf_light.name.c_str(), id++, pf_light, LightsCollectionView);
					}
					ImGui::EndChild();
				}
				ImGui::SameLine();

				// -- Right side, when selecting a light
				if (LightsCollectionView.selectedLight)
				{
					ImGui::BeginGroup();
					ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())); // Leave room for 1 line below us
					ImGui::Text(LightsCollectionView.selectedLight->name.c_str());
					ImGui::Separator();
					inspectLight(*LightsCollectionView.selectedLight);
					ImGui::EndChild(); 
					ImGui::EndGroup(); 
				}
				

				ImGui::End();
			}
		
			void inspectLight(const pf_Light& light) {

				if (!light.sourceLight) return;

				ImGui::Checkbox("IsOn", &light.sourceLight->isOn);
				ImGui::Separator();
				ImGui::Checkbox("Cast dynamic shadows", (bool*)&light.sourceLight->shadowMode);
				if (light.sourceLight->shadowMode == pyr::DynamicShadow && light.sourceLight->getType() != pyr::LightTypeID::Point)
				{
					ImGui::Text("Projection Parameters");
					if (pyr::DirectionalLight* asDirectional = static_cast<pyr::DirectionalLight*>(light.sourceLight))
					{
						ImGui::SliderFloat("Width", &asDirectional->shadow_projection.width, 1.F, 1000.F);
						ImGui::SliderFloat("Height", &asDirectional->shadow_projection.height, 1.F, 1000.F);
						ImGui::SliderFloat("zNear", &asDirectional->shadow_projection.zNear, 0.01F, 1.F);
						ImGui::SliderFloat("zFar", &asDirectional->shadow_projection.zFar,  1.F, 100.F);
					}
					if (pyr::SpotLight* asSpotlight = static_cast<pyr::SpotLight*>(light.sourceLight))
					{
						ImGui::SliderFloat("Fov", &asSpotlight->shadow_projection.fovy, 0.01f, XM_PI);
						ImGui::SliderFloat("zNear", &asSpotlight->shadow_projection.zNear, 0.01F, 1.F);
						ImGui::SliderFloat("zFar", &asSpotlight->shadow_projection.zFar, 1.1F, 100.F);
					}
				}
				ImGui::Separator();
				ImGui::ColorEdit3("Ambiant", &light.sourceLight->ambiant.x);
				ImGui::ColorEdit3("Diffuse", &light.sourceLight->diffuse.x);
				ImGui::Separator();
				switch (light.sourceLight->getType())
				{
				case pyr::LightTypeID::Directional:
				{
					pyr::DirectionalLight* sourceLight = static_cast<pyr::DirectionalLight*>(light.sourceLight);
					if (!sourceLight) break;
					ImGui::DragFloat3("Direction", &sourceLight->GetTransform().rotation.x);
					ImGui::DragFloat("Strength", &sourceLight->strength, 1.0, 0);
					break;
				}
				case pyr::LightTypeID::Spotlight:
				{
					pyr::SpotLight* sourceLight = static_cast<pyr::SpotLight*>(light.sourceLight);
					if (!sourceLight) break;
					ImGui::DragFloat3("Position", &sourceLight->GetTransform().position.x);
					ImGui::DragFloat3("Direction", &sourceLight->GetTransform().rotation.x);
					ImGui::DragFloat("Strength", &sourceLight->strength);
					if (ImGui::DragFloat("Hard light angle", &sourceLight->insideAngle, 0.05f, 0.f, XM_PI) +
						ImGui::DragFloat("Fall-off angle", &sourceLight->outsideAngle, 0.05f, 0.0f, XM_PI))
					{
						sourceLight->shadow_projection.fovy = std::clamp<float>((sourceLight->insideAngle + sourceLight->outsideAngle) * 2.F, 0.01f, XM_PI);
					}
					ImGui::DragFloat("SpecularFactor", &sourceLight->specularFactor, 1.0f, 0.F);
					break;
				}
				case pyr::LightTypeID::Point:
				{
					pyr::PointLight* sourceLight = static_cast<pyr::PointLight*>(light.sourceLight);
					if (!sourceLight) break;
					if (ImGui::DragInt("Distance", &sourceLight->distance, 1, 0, 11))
					{
						sourceLight->range = sourceLight->computeRangeFromDistance(sourceLight->distance);
					}
					ImGui::DragFloat3("Position", &sourceLight->GetTransform().position.x);
					ImGui::DragFloat("specularFactor", &sourceLight->specularFactor, 1.0, 0);
					ImGui::Separator();

					break;
				}
				}

			}

		};
		
	}
}