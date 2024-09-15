#pragma once

#include "imgui.h"

#include "display/texture.h"
#include "editor/bridges/Lights/pf_Light.h"
#include "editor/views/widget.h"


namespace pye
{
	namespace widgets
	{

		struct LightCollectionWidget : public Widget
		{

			mutable pf_LightsCollection LightsCollectionView;

			virtual void display() override
			{
				if (!LightsCollectionView.sourceCollection) return;
				LightsCollectionView.UpdateEditorFormat();

				static pyr::Texture AddIcon = getWidgetAssetRegistry().loadTexture(L"res/editor/plus.png");
				static pyr::Texture RemoveIcon = getWidgetAssetRegistry().loadTexture(L"res/editor/retirer.png");
				static auto makeSelectable = [](const char* label, int id, pf_Light* ref, pf_LightsCollection &LightsCollectionView)
				{
						ImGui::PushID(label);
						if (ImGui::Selectable(label, LightsCollectionView.selectedId == id))
						{
							LightsCollectionView.selectedId = id;
							LightsCollectionView.selectedLight = ref;
						}
						ImGui::PopID();

				};

				// -- Start the widget
				ImGui::Begin("Lights");
				{

					// Add the plus sign
					//if (ImGui::BeginMenuBar())
					if (ImGui::BeginPopupContextItem("AddLightPopup"))
					{
						if (ImGui::Selectable("Point light"))
						{
							LightsCollectionView.sourceCollection->Points.push_back({});
							LightsCollectionView.UpdateEditorFormat();
						}
						if (ImGui::Selectable("Spot light")) 
						{
							LightsCollectionView.sourceCollection->Directionals.push_back({}); 
							LightsCollectionView.UpdateEditorFormat();
						}
						if (ImGui::Selectable("Directional light"))
						{
							LightsCollectionView.sourceCollection->Directionals.push_back({});
							LightsCollectionView.UpdateEditorFormat();
						}
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
								LightsCollectionView.UpdateEditorFormat();
							}
						}
						ImGui::PopStyleColor();
					}
				}
				ImGui::Separator();
				{
					ImGui::BeginChild("left pane", ImVec2(150, 0), ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX);
					int id = 0;
					for (pf_Light& pf_light : LightsCollectionView.EditorFormat)
					{
						makeSelectable(pf_light.name.c_str(), id++, &pf_light, LightsCollectionView);
					}
					ImGui::EndChild();
				}
				ImGui::SameLine();

				// Right
				
				if (LightsCollectionView.selectedLight)
				{
					ImGui::BeginGroup();
					ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())); // Leave room for 1 line below us
					ImGui::Text(LightsCollectionView.selectedLight->name.c_str());
					ImGui::Separator();
					inspectLight(LightsCollectionView.selectedLight);
					ImGui::EndChild(); 
					ImGui::EndGroup(); 
				}
				

				ImGui::End();
			}
		
			void inspectLight(pf_Light* light) {

				if (!light || !light->sourceLight) return;

				ImGui::Checkbox("IsOn", &light->sourceLight->isOn);
				ImGui::Separator();
				ImGui::ColorEdit3("Ambiant", &light->sourceLight->ambiant.x);
				ImGui::ColorEdit3("Diffuse", &light->sourceLight->diffuse.x);
				ImGui::Separator();
				switch (light->type)
				{
				case pf_Light::DIR:
				{
					pyr::DirectionalLight* sourceLight = static_cast<pyr::DirectionalLight*>(light->sourceLight);
					if (!sourceLight) break;
					ImGui::DragFloat3("Direction", &sourceLight->direction.x);
					ImGui::DragFloat("Strength", &sourceLight->strength, 1.0, 0);
					break;
				}
				case pf_Light::SPOT:
				{
					pyr::SpotLight* sourceLight = static_cast<pyr::SpotLight*>(light->sourceLight);
					if (!sourceLight) break;
					ImGui::DragFloat3("Position", &sourceLight->position.x);
					ImGui::DragFloat3("Direction", &sourceLight->direction.x);
					ImGui::DragFloat("Strength", &sourceLight->strength);
					ImGui::DragFloat("Inside angle", &sourceLight->insideAngle, 0.05f, 0.f, XM_2PI);
					ImGui::DragFloat("Outside angle", &sourceLight->outsideAngle, 0.05f, 0.f, XM_2PI);
					ImGui::DragFloat("SpecularFactor", &sourceLight->specularFactor, 1.0f, 0.F);
					break;
				}
				case pf_Light::POINT:
				{
					pyr::PointLight* sourceLight = static_cast<pyr::PointLight*>(light->sourceLight);
					if (!sourceLight) break;
					if (ImGui::DragInt("Distance", &sourceLight->distance, 1, 0, 11))
					{
						sourceLight->range = sourceLight->computeRangeFromDistance(sourceLight->distance);
					}
					ImGui::DragFloat3("Position", &sourceLight->position.x);
					ImGui::DragFloat("specularFactor", &sourceLight->specularFactor, 1.0, 0);
					ImGui::Separator();

					break;
				}
				}

			}

		};
		
	}
}