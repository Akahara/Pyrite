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
				static auto makeSelectable = [&](const char* label, int id, pf_Light* ref)
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
						makeSelectable(pf_light.name.c_str(), id++, &pf_light);
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
					ImGui::DragFloat("Inside angle", &sourceLight->insideAngle, 0.05, 0, XM_2PI);
					ImGui::DragFloat("Outside angle", &sourceLight->outsideAngle, 0.05, 0, XM_2PI);
					ImGui::DragFloat("SpecularFactor", &sourceLight->specularFactor, 1.0, 0);
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


		//std::optional<hlsl_GenericLight> showDebugWindow()
		//{
		//	ImGui::Begin("Lights");
		//	if (ImGui::Button("Add new point light"))
		//	{
		//
		//		m_point.push_back(PointLight{ 3 ,{}, {1,1,1}, {1,1,1}, 1, true });
		//	}
		//	if (ImGui::Button("Add new spot light"))
		//	{
		//		SpotLight sl;
		//		sl.insideAngle = 1;
		//		sl.direction = { 0,-1,0 };
		//		sl.position = {};
		//		sl.outsideAngle = 1.f;
		//		sl.strength = 1.F;
		//		sl.ambiant = { 1,1,1 };
		//		sl.diffuse = { 1,1,1 };
		//		sl.specularFactor = 1.F;
		//		m_spots.push_back(sl);
		//	}
		//
		//
		//
		//	static std::vector<std::string> names;
		//	static std::string lastName = "No light selected";
		//	names = getAllLightsDebugName();
		//	names.insert(names.begin() + 0, "None");
		//	static const char* current_item = nullptr;
		//	static int currentItemIndex = -1;
		//	current_item = lastName.c_str();
		//
		//	if (names.empty())
		//	{
		//		ImGui::End();
		//		return std::nullopt;
		//	}
		//
		//	if (ImGui::BeginCombo("All lights", current_item, ImGuiComboFlags_NoArrowButton))
		//	{
		//		for (int n = 0; n < names.size(); n++)
		//		{
		//			bool is_selected = (currentItemIndex == n); // You can store your selection however you want, outside or inside your objects
		//			if (ImGui::Selectable(names[n].c_str(), is_selected))
		//			{
		//				currentItemIndex = n;
		//				lastName = names[n];
		//			}
		//			if (is_selected)
		//				ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
		//		}
		//		ImGui::EndCombo();
		//	}
		//
		//	std::optional<hlsl_GenericLight> returnLight = std::nullopt;
		//	if (currentItemIndex >= 0 && !names.empty())
		//	{
		//		ImGui::BeginChild("Current Selected");
		//
		//		switch (names[currentItemIndex][0])
		//		{
		//		case 'N':
		//			break;
		//		case 'P':
		//		{
		//			size_t tmpIndex = currentItemIndex - 1;
		//			ImGui::DragFloat3(("Position ##" + std::to_string(tmpIndex)).c_str(), &m_point[tmpIndex].position.vector4_f32[0]);
		//			if (ImGui::DragInt(("Distance ##" + std::to_string(tmpIndex)).c_str(), &m_point[tmpIndex].distance, 1, 0, 11))
		//			{
		//				m_point[tmpIndex].range = computeRangeFromDistance(m_point[tmpIndex].distance);
		//
		//			}
		//			ImGui::BeginDisabled();
		//			ImGui::DragFloat4(("Range ##" + std::to_string(tmpIndex)).c_str(), m_point[tmpIndex].range.vector4_f32);
		//			ImGui::EndDisabled();
		//			ImGui::DragFloat(("specularFactor ##" + std::to_string(tmpIndex)).c_str(), &m_point[tmpIndex].specularFactor);
		//			ImGui::DragFloat3(("Ambiant ##" + std::to_string(tmpIndex)).c_str(), &m_point[tmpIndex].ambiant.vector4_f32[0]);
		//			ImGui::DragFloat3(("Diffuse ##" + std::to_string(tmpIndex)).c_str(), &m_point[tmpIndex].diffuse.vector4_f32[0]);
		//			ImGui::Checkbox(("IsOn ##" + std::to_string(tmpIndex)).c_str(), &m_point[tmpIndex].isOn);
		//			returnLight = pointToGeneric(m_point[tmpIndex]);
		//			break;
		//		}
		//		case 'S':
		//		{
		//
		//			size_t tmpIndex = currentItemIndex - m_point.size() - 1;
		//			SpotLight& sl = m_spots[tmpIndex];
		//
		//
		//
		//			ImGui::DragFloat3(("Position ##" + std::to_string(currentItemIndex)).c_str(), &sl.position.vector4_f32[0]);
		//			if (ImGui::DragFloat3(("Direction ##" + std::to_string(currentItemIndex)).c_str(), &sl.direction.vector4_f32[0], 0.01f, -1.f, 1.f))
		//			{
		//				sl.direction = XMVector3Normalize(sl.direction);
		//			}
		//			ImGui::DragFloat3(("Ambiant ##" + std::to_string(currentItemIndex)).c_str(), &sl.ambiant.vector4_f32[0]);
		//			ImGui::DragFloat3(("Diffuse ##" + std::to_string(currentItemIndex)).c_str(), &sl.diffuse.vector4_f32[0]);
		//
		//			ImGui::DragFloat(("specularFactor ##" + std::to_string(currentItemIndex)).c_str(), &sl.specularFactor);
		//			ImGui::DragFloat(("Inside Angle (radius) ##" + std::to_string(currentItemIndex)).c_str(), &sl.insideAngle, 0.01f, 0.F, XM_PIDIV2);
		//			ImGui::DragFloat(("Outside angle (falloff) ##" + std::to_string(currentItemIndex)).c_str(), &sl.outsideAngle, 0.01f, 0.F, XM_PIDIV2);
		//			ImGui::DragFloat(("Strength ##" + std::to_string(currentItemIndex)).c_str(), &sl.strength);
		//
		//			ImGui::Checkbox(("IsOn ##" + std::to_string(currentItemIndex)).c_str(), &m_spots[tmpIndex].isOn);
		//
		//			returnLight = spotToGeneric(sl);
		//
		//			break;
		//
		//		}
		//		case 'D':
		//			break;
		//		default:
		//			break;
		//		}
		//
		//		ImGui::EndChild();
		//	}
		//
		//	ImGui::End();
		//	return returnLight;
		//}
	}
}