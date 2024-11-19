#pragma once

#include "imgui.h"

#include "display/texture.h"
#include "display/FrameBuffer.h"	
#include "display/RenderGraph/BuiltinPasses/ForwardPass.h"	

#include "world/Mesh/StaticMesh.h"
#include "world/Mesh/Model.h"
#include "world/Mesh/MeshImporter.h"
#include "world/Tools/CommonConstantBuffers.h"

#include "editor/bridges/Lights/pf_Light.h"
#include "editor/views/widget.h"
#include "editor/Editor.h"
#include "editor/EditorEvents.h"

#include <utils/math.h>

inline constexpr float POLAR_CAP = 90.0f - 0.1f;
inline constexpr float sensitivity = 0.35f;


namespace pye
{
	namespace widgets
	{

		struct MaterialWidget : public Widget
		{

		private:
		
			std::vector<std::shared_ptr<pyr::Model>> m_model = pyr::MeshImporter::ImportMeshesFromFile(L"res/meshes/editor/displayBall/ball.gltf");
			pyr::StaticMesh displayBall;
			std::shared_ptr<pyr::Material> m_materialToDisplay;
		
			// -- Rendering 
			pyr::FrameBuffer framebuffer{ 512,512, pyr::FrameBuffer::COLOR_0 };
			pyr::Camera renderCamera;
			pyr::DirectionalLight light;
			float zoom = 3.f;

			std::shared_ptr<pyr::CameraBuffer>  pcameraBuffer = std::make_shared<pyr::CameraBuffer>();
		
			pyr::RenderGraph renderGraph;
			pyr::BuiltinPasses::ForwardPass     m_forwardPass;
			pyr::BuiltinPasses::DepthPrePass    m_depthPrePass{512,512};
			pyr::RegisteredRenderableActorCollection toRender;
			
			// TODO : environement here
			size_t selectedMaterialID = 0;

		
		
		
		public:
		
			MaterialWidget()
			{
				
				renderCamera.setProjection(pyr::PerspectiveProjection{ .fovy = XM_PIDIV2, .aspect = 1.F,.zNear = 0.01f,  .zFar = 1000.F });
		
				displayBall = pyr::StaticMesh{ m_model[0] };
				displayBall.GetTransform().position = { 0,0,0 };
		
				renderGraph.addPass(&m_depthPrePass);
				renderGraph.addPass(&m_forwardPass);
		
				renderGraph.getResourcesManager().addProduced(&m_depthPrePass, "depthBuffer");
				renderGraph.getResourcesManager().linkResource(&m_depthPrePass, "depthBuffer", &m_forwardPass);
		
				renderGraph.getResourcesManager().checkResourcesValidity();
				toRender.meshes.push_back(&displayBall);
				toRender.lights.Directionals.push_back(light);


				pye::EditorEvents::OnActorPickedEvent.BindCallback(*this, &MaterialWidget::ChangeMaterialFromNewlySelectedActor);
			}
		
			virtual void display() override 
			{
		
				DrawIntoFramebuffer();
		
				ImGui::Begin("Material displayer");
				ImGui::SeparatorText("Material infos");


				if (ImGui::BeginCombo("Material##1", pyr::MaterialBank::GetMaterialName(selectedMaterialID).c_str() ))
				{
					for (auto [id, ref] : pyr::MaterialBank::GetAllMaterials())
					{
						ImGui::PushID((int)id);
						if (ImGui::Selectable((ref->d_publicName + "##" + std::to_string(id)).c_str(), id == selectedMaterialID))
						{
							selectedMaterialID = id;
							ChangeMaterial(id);

						}
						if (id == static_cast<int>(selectedMaterialID))
							ImGui::SetItemDefaultFocus();
						ImGui::PopID();
					}
					ImGui::EndCombo();
				}

				if (m_materialToDisplay)
				{
					ImGui::SeparatorText("Parameters");
					auto& coefs = m_materialToDisplay->getMaterialRenderingCoefficients();

					ImGui::SliderFloat4("Albedo", &coefs.Ka.x, 0.f, 1.F);
					ImGui::SliderFloat4("Specular", &coefs.Ks.x, 0.f, 1.F);
					ImGui::SliderFloat4("Emissive", &coefs.Ke.x, 0.f, 1.F);
					ImGui::SliderFloat("Roughness", &coefs.Roughness, 0.f, 1.F);
					ImGui::SliderFloat("Metallic", &coefs.Metallic, 0.f, 1.F);
					ImGui::SliderFloat("Optical Density", &coefs.Ni, 0.f, 1.F);
				}

				ImGui::Separator();
				if (ImGui::ImageButton((void*)framebuffer.getTargetAsTexture(pyr::FrameBuffer::COLOR_0).getRawTexture(), ImVec2{ 300,300 }))
				{

				}
				auto& io = ImGui::GetIO();
				static bool first = true;
				if (ImGui::IsItemHovered() || first)
				{
					first = false;
					static float azimuth_ = 0, altitude_ = 0;

					// for some reasons, imgui scroll is not working
					const pyr::MouseState& mouse = pyr::UserInputs::getMouseState();
					zoom = std::clamp<float>(zoom - mouse.deltaScroll * 0.001F, 1.f, 3.F);

					if (ImGui::IsItemActive())
					{
						ImVec2 mouse_delta = io.MouseDelta;
						azimuth_ = (float)std::fmod<float>(azimuth_ - mouse_delta.x * sensitivity, 360.0f);
						altitude_ = std::clamp<float>(altitude_ + mouse_delta.y * sensitivity, -POLAR_CAP, POLAR_CAP);
					}

					vec3 cameraPosition;
					cameraPosition.x = zoom * cos(XMConvertToRadians(altitude_)) * cos(XMConvertToRadians(azimuth_));
					cameraPosition.y = zoom * sin(XMConvertToRadians(altitude_));
					cameraPosition.z = zoom * cos(XMConvertToRadians(altitude_)) *	sin(XMConvertToRadians(azimuth_));

					renderCamera.setPosition(cameraPosition);
					renderCamera.lookAt({0,0,0});
				}
				ImGui::End();
			
			};
		
			void ChangeMaterial(pyr::MaterialBank::mat_id_t materialGlobalId)
			{
				auto matRef = pyr::MaterialBank::GetMaterialReference(materialGlobalId);
				m_materialToDisplay = matRef;
				displayBall.overrideSubmeshMaterial(0, matRef);
				selectedMaterialID = materialGlobalId;
			}
		
		private:
		
			void ChangeMaterialFromNewlySelectedActor(pye::EditorActor* editorActor)
			{
				auto* asMesh = dynamic_cast<pye::pf_StaticMesh*>(editorActor);
				if (!asMesh) return; // < ignore non mesh actors

				PYR_ENSURE(asMesh->sourceMesh, "Editor-side mesh has no core-side mesh. Wtf ?");

				// Current implementation is kinda stupid and each submesh is a mesh so this is a hack until we redo the import correctly for the 10th time
				size_t firstSubmeshMaterial = asMesh->sourceMesh->getModel()->getRawMeshData()->getSubmeshes()[0].materialIndex;
				if (auto material = asMesh->sourceMesh->getMaterial(firstSubmeshMaterial))
				{
					ChangeMaterial(pyr::MaterialBank::GetMaterialGlobalId(material->d_publicName)); // i dont like this being name based
				}
			}

			void DrawIntoFramebuffer()
			{
		
				pyr::Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				pyr::RenderProfiles::pushRasterProfile(pyr::RasterizerProfile::CULLBACK_RASTERIZER);
				pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTWRITE_DEPTH);
				framebuffer.bind();

				pcameraBuffer->setData(pyr::CameraBuffer::data_t{
				   .mvp = renderCamera.getViewProjectionMatrix(),
				   .pos = renderCamera.getPosition()
				});
				m_depthPrePass.getDepthPassEffect()->bindConstantBuffer("CameraBuffer", pcameraBuffer);
				m_forwardPass.getSkyboxEffect()->bindConstantBuffer("CameraBuffer", pcameraBuffer);

				renderCamera.lookAt({ 0,0,0 });
				renderGraph.execute(pyr::RenderContext{ toRender , "Material Display Widget ", &renderCamera });
				framebuffer.unbind();
				pyr::RenderProfiles::popDepthProfile();
				pyr::RenderProfiles::popRasterProfile();
			}

		};
	}
}