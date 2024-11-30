#pragma once

#include "display/RenderGraph/RenderPass.h"
#include "display/RenderGraph/RenderGraph.h"
#include "display/GraphicalResource.h"
#include "world/Mesh/RawMeshData.h"
#include "world/Mesh/StaticMesh.h"
#include "world/Tools/CommonConstantBuffers.h"
#include "display/FrameBuffer.h"
#include "display/RenderProfiles.h"

#include "world/Shadows/ReflectiveShadowMap.h"

#include <ranges>

namespace pyr
{
    // -- Definitely not an optimal way, but i want this pass to produce a texture of indirect lighting that we will composition after
    namespace BuiltinPasses
    {

        class ReflectiveShadowMap_GlobalIlluminationPass : public RenderPass
        {
        private:

            pyr::GraphicalResourceRegistry m_registry;
            pyr::FrameBuffer m_indirectLighting{512,512, pyr::FrameBuffer::COLOR_0 | pyr::FrameBuffer::DEPTH_STENCIL };
            pyr::FrameBuffer m_lowResPass{64, 64, pyr::FrameBuffer::COLOR_0 | pyr::FrameBuffer::DEPTH_STENCIL };
            pyr::FrameBuffer m_blurTarget{pyr::FrameBuffer::COLOR_0};
            pyr::Effect* computeIndirectLightingEffect;
            pyr::Effect* m_blurEffect;

            std::shared_ptr<CameraBuffer>    pcameraBuffer  = std::make_shared<CameraBuffer>()  ;
            std::shared_ptr<ActorBuffer>     pActorBuffer   = std::make_shared<ActorBuffer>()   ;
           //std::shared_ptr<LightsBuffer>    pLightBuffer   = std::make_shared<LightsBuffer>()  ;

            using SingleLightBuffer = pyr::ConstantBuffer < InlineStruct(alignas(16) pyr::hlsl_GenericLight light) >;
            std::shared_ptr<SingleLightBuffer>    pLightBuffer = std::make_shared<SingleLightBuffer>();

            float u_DistanceThreshold = 0.05f;
            float u_NormalThreshold = 0.95f;
            float u_Rmax = 0.3f;
        
        public:


            ReflectiveShadowMap_GlobalIlluminationPass()
            {

                displayName = "RSM GI Pass";
                computeIndirectLightingEffect = m_registry.loadEffect(L"res/shaders/rsm_evaluate.hlsl", InputLayout::MakeLayoutFromVertex<pyr::RawMeshData::mesh_vertex_t>());
                m_blurEffect = m_registry.loadEffect(L"res/shaders/gaussianBlur.fx", InputLayout::MakeLayoutFromVertex<EmptyVertex>());

                producesResource("GI_CompositeIndirectIllumination", m_indirectLighting.getTargetAsTexture(pyr::FrameBuffer::COLOR_0));

            }


            virtual void apply() override
            {
                PYR_ENSURE(owner);
                if (!PYR_ENSURE(owner->GetContext().contextCamera)) return;
                pcameraBuffer->setData(CameraBuffer::data_t{ .mvp = owner->GetContext().contextCamera->getViewProjectionMatrix(), .pos = owner->GetContext().contextCamera->getPosition() });
                
                // -- Create raw light buffer
                pyr::LightsCollections& lights = owner->GetContext().ActorsToRender.lights;
                //LightsBuffer::data_t light_data{};
                //std::copy_n(lights.ConvertCollectionToHLSL().begin(), std::size(light_data.lights), std::begin(light_data.lights));
                //pLightBuffer->setData(light_data);
                auto castsShadows = [](const pyr::BaseLight* light) -> bool { return light->isOn && light->shadowMode == pyr::DynamicShadow_RSM; };
                auto filtered = lights.Spots | std::ranges::views::filter([&castsShadows](const pyr::SpotLight& spotlight) { return castsShadows(&spotlight); });
                if (filtered.empty()) return;
                pLightBuffer->setData(SingleLightBuffer::data_t{ .light = convertLightTo_HLSL(lights.Spots[0]) });


                Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTWRITE_DEPTH);

                ///////// ---------------- ///////// 

                computeIndirectLightingEffect->setUniform<float>("u_DistanceComparisonThreshold", u_DistanceThreshold);
                computeIndirectLightingEffect->setUniform<float>("u_NormalComparisonThreshold", u_NormalThreshold);
                computeIndirectLightingEffect->setUniform<float>("u_Rmax", u_Rmax);

                // 1. low res pass
                m_lowResPass.clearTargets();
                m_lowResPass.bind();
                computeIndirectLightingEffect->bindTexture(pyr::Texture::getDefaultTextureSet().BlackPixel, "LowResTexture");
                computeIndirectLightingEffect->setUniform<int>("u_CurrentPassID", 0);
                RenderFn();
                m_lowResPass.unbind();


                // 1. 3 high quality passes using interpolation
                m_indirectLighting.clearTargets();
                m_indirectLighting.bind();
                computeIndirectLightingEffect->setUniform<vec2>("u_FullTextureDimensions", m_indirectLighting.GetDimensions());
                computeIndirectLightingEffect->bindTexture(m_lowResPass.getTargetAsTexture(pyr::FrameBuffer::COLOR_0), "LowResTexture");
                for (int i = 0; i < 3; i++)
                {
                    computeIndirectLightingEffect->setUniform<int>("u_CurrentPassID", i + 1);
                    RenderFn();
                }
                m_indirectLighting.unbind();
                
                
                
                ///////// ---------------- ///////// 
                
                pyr::RenderProfiles::popDepthProfile();


            }

            void RenderFn()
            {
                computeIndirectLightingEffect->bindConstantBuffer("CameraBuffer", pcameraBuffer);
                computeIndirectLightingEffect->bindConstantBuffer("SingleLightBuffer", pLightBuffer);
                computeIndirectLightingEffect->bindTexture(std::get<TextureArray>(owner->getResourcesManager().fetchResource("RSM_WorldPos_TextureArray")), "RSM_WorldPositions");
                computeIndirectLightingEffect->bindTexture(std::get<TextureArray>(owner->getResourcesManager().fetchResource("RSM_Normals_TextureArray")), "RSM_Normals");
                computeIndirectLightingEffect->bindTexture(std::get<TextureArray>(owner->getResourcesManager().fetchResource("RSM_Flux_TextureArray")), "RSM_Flux");

                computeIndirectLightingEffect->bindTexture(std::get<TextureArray>(owner->getResourcesManager().fetchResource("RSM_LowRes_WorldPos_TextureArray")), "RSM_LowRes_WorldPositions");
                computeIndirectLightingEffect->bindTexture(std::get<TextureArray>(owner->getResourcesManager().fetchResource("RSM_LowRes_Normals_TextureArray")), "RSM_LowRes_Normals");
                computeIndirectLightingEffect->bindTexture(std::get<TextureArray>(owner->getResourcesManager().fetchResource("RSM_LowRes_Flux_TextureArray")), "RSM_LowRes_Flux");

                for (const StaticMesh* smesh : owner->GetContext().ActorsToRender.meshes)
                {

                    smesh->bindModel();

                    pActorBuffer->setData(ActorBuffer::data_t{ .modelMatrix = smesh->GetTransform().getWorldMatrix() });
                    computeIndirectLightingEffect->bindConstantBuffer("ActorBuffer", pActorBuffer);
                    computeIndirectLightingEffect->bind();

                    std::span<const SubMesh> submeshes = smesh->getModel()->getRawMeshData()->getSubmeshes();
                    for (auto& submesh : submeshes)
                    {
                        Engine::d3dcontext().DrawIndexed(static_cast<UINT>(submesh.getIndexCount()), submesh.startIndex, 0);
                    }

                    computeIndirectLightingEffect->unbindResources();
                }

            }

            virtual bool HasDebugWindow() override { return true; }
            virtual void OpenDebugWindow() override 
            {
                ImGui::Begin("RSM Gi Debug window");

                if (ImGui::SliderFloat("Distance comparison threshold", &u_DistanceThreshold, 0, 10.F) +
                    ImGui::SliderFloat("Normal comparison threshold", &u_NormalThreshold, 0, 1.F) +
                    ImGui::SliderFloat("Sampling disk max radius", &u_Rmax, 0, 1.F)
                    )
                {
                    computeIndirectLightingEffect->setUniform<float>("u_DistanceComparisonThreshold", u_DistanceThreshold);
                    computeIndirectLightingEffect->setUniform<float>("u_NormalComparisonThreshold", u_NormalThreshold);
                    computeIndirectLightingEffect->setUniform<float>("u_Rmax", u_Rmax);
                }

                ImGui::End();
            }

        };
    }
}
