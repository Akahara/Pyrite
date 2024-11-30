#pragma once

#include "display/RenderGraph/RenderPass.h"
#include "display/RenderGraph/RenderGraph.h"
#include "display/GraphicalResource.h"
#include "world/Mesh/RawMeshData.h"
#include "world/Mesh/StaticMesh.h"
#include "world/Tools/CommonConstantBuffers.h"
#include "display/FrameBuffer.h"
#include "display/RenderProfiles.h"

#include "world/Lights/Light.h"

#include "world/Shadows/ReflectiveShadowMap.h"

#include <ranges>

namespace pyr
{
    // -- Look up the amount of lights that want shadow map, then compute the appropriate buffer and store it.
    namespace BuiltinPasses
    {

        class ReflectiveShadowMapComputePass : public RenderPass
        {
        private:

            pyr::GraphicalResourceRegistry m_registry;

            std::shared_ptr<ActorBuffer>     pActorBuffer = std::make_shared<ActorBuffer>();
            std::shared_ptr<CameraBuffer>    pcameraBuffer = std::make_shared<CameraBuffer>();

            using SingleLightBuffer = pyr::ConstantBuffer < InlineStruct(alignas(16) pyr::hlsl_GenericLight light) > ;
            std::shared_ptr<SingleLightBuffer>    pLightBuffer = std::make_shared<SingleLightBuffer>();

            pyr::Effect* RSMComputeEffect = m_registry.loadEffect(L"res/shaders/rsm.hlsl", InputLayout::MakeLayoutFromVertex<pyr::RawMeshData::mesh_vertex_t>());

            static constexpr int MAX_RSM_COUNT = 8;
            static constexpr unsigned int RESOLUTION = 128;
            static constexpr unsigned int LOW_RESOLUTION = 32;

            TextureArray RSM_WorldPos_TextureArray      { RESOLUTION,RESOLUTION, MAX_RSM_COUNT, TextureArray::Texture2D };
            TextureArray RSM_Normals_TextureArray       { RESOLUTION,RESOLUTION, MAX_RSM_COUNT, TextureArray::Texture2D };
            TextureArray RSM_Flux_TextureArray          { RESOLUTION,RESOLUTION, MAX_RSM_COUNT, TextureArray::Texture2D };
            TextureArray RSM_DepthBuffers_TextureArray  { RESOLUTION,RESOLUTION, MAX_RSM_COUNT, TextureArray::Texture2D, true };

            TextureArray RSM_LowRes_WorldPos_TextureArray{ LOW_RESOLUTION,LOW_RESOLUTION, MAX_RSM_COUNT, TextureArray::Texture2D };
            TextureArray RSM_LowRes_Normals_TextureArray{ LOW_RESOLUTION,LOW_RESOLUTION, MAX_RSM_COUNT, TextureArray::Texture2D };
            TextureArray RSM_LowRes_Flux_TextureArray{ LOW_RESOLUTION,LOW_RESOLUTION, MAX_RSM_COUNT, TextureArray::Texture2D };
            TextureArray RSM_LowRes_DepthBuffers_TextureArray{ LOW_RESOLUTION,LOW_RESOLUTION, MAX_RSM_COUNT, TextureArray::Texture2D, true };

        public:

            ReflectiveShadowMapComputePass(unsigned int width, unsigned int height) 
            {
                displayName = "RSM Compute";

                producesResource("RSM_WorldPos_TextureArray", RSM_WorldPos_TextureArray);
                producesResource("RSM_Normals_TextureArray", RSM_Normals_TextureArray);
                producesResource("RSM_Flux_TextureArray", RSM_Flux_TextureArray);
                producesResource("RSM_DepthBuffers_TextureArray", RSM_DepthBuffers_TextureArray);

                producesResource("RSM_LowRes_WorldPos_TextureArray",        RSM_LowRes_WorldPos_TextureArray);
                producesResource("RSM_LowRes_Normals_TextureArray",         RSM_LowRes_Normals_TextureArray);
                producesResource("RSM_LowRes_Flux_TextureArray",            RSM_LowRes_Flux_TextureArray);
                producesResource("RSM_LowRes_DepthBuffers_TextureArray",    RSM_LowRes_DepthBuffers_TextureArray);
            }


            ReflectiveShadowMapComputePass() : ReflectiveShadowMapComputePass(pyr::Device::getWinWidth(), pyr::Device::getWinHeight())
            {}

            virtual void apply() override
            {
                if (!PYR_ENSURE(owner)) return;
                if (!PYR_ENSURE(owner->GetContext().contextCamera)) return;




                // -- Fetch the lights that want to cast create RSM (which would be the one with shadowMode == DynamicShadow_RSM)

                /// Only spot lights for now, easier for implementation
                pyr::LightsCollections& lights = owner->GetContext().ActorsToRender.lights;
                auto castsShadows = [](const pyr::BaseLight* light) -> bool { return light->isOn && light->shadowMode == pyr::DynamicShadow_RSM; };
                auto filtered = lights.Spots | std::ranges::views::filter([&castsShadows](const pyr::SpotLight& spotlight) { return castsShadows(&spotlight); });
                if (filtered.empty()) return;

                static ReflectiveShadowMap rsm{ RESOLUTION,RESOLUTION, lights.Spots.data() };
                static ReflectiveShadowMap rsm_low{ LOW_RESOLUTION,LOW_RESOLUTION, lights.Spots.data() };
                rsm_low.GetFramebuffer().clearTargets();
                rsm.GetFramebuffer().clearTargets();

                pyr::Camera camera{};
                pyr::SpotLight& light = lights.Spots[0];
                camera.setProjection(light.shadow_projection);
                camera.setPosition(light.GetTransform().position);
                vec3 fuck = { light.GetTransform().rotation.x, light.GetTransform().rotation.y, light.GetTransform().rotation.z };
                camera.lookAt(light.GetTransform().position + fuck);

                pcameraBuffer->setData(CameraBuffer::data_t{ .mvp = camera.getViewProjectionMatrix(), .pos = camera.getPosition() });
                pLightBuffer->setData(SingleLightBuffer::data_t{ .light = convertLightTo_HLSL(light) });



                Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTWRITE_DEPTH);




                rsm_low.GetFramebuffer().bind();
                RenderFn();
                rsm_low.GetFramebuffer().unbind();

                rsm.GetFramebuffer().bind();
                RenderFn();
                rsm.GetFramebuffer().unbind();

                TextureArray::CopyToTextureArray({ rsm.GetFramebuffer().getTargetAsTexture(ReflectiveShadowMap::Targets::WorldPos) }, RSM_WorldPos_TextureArray);
                TextureArray::CopyToTextureArray({ rsm.GetFramebuffer().getTargetAsTexture(ReflectiveShadowMap::Targets::Normal) }, RSM_Normals_TextureArray);
                TextureArray::CopyToTextureArray({ rsm.GetFramebuffer().getTargetAsTexture(ReflectiveShadowMap::Targets::Flux) }, RSM_Flux_TextureArray);
                TextureArray::CopyToTextureArray({ rsm.GetFramebuffer().getTargetAsTexture(ReflectiveShadowMap::Targets::Depth) }, RSM_DepthBuffers_TextureArray);


                TextureArray::CopyToTextureArray({ rsm_low.GetFramebuffer().getTargetAsTexture(ReflectiveShadowMap::Targets::WorldPos) },   RSM_LowRes_WorldPos_TextureArray);
                TextureArray::CopyToTextureArray({ rsm_low.GetFramebuffer().getTargetAsTexture(ReflectiveShadowMap::Targets::Normal) },     RSM_LowRes_Normals_TextureArray);
                TextureArray::CopyToTextureArray({ rsm_low.GetFramebuffer().getTargetAsTexture(ReflectiveShadowMap::Targets::Flux) },       RSM_LowRes_Flux_TextureArray);
                TextureArray::CopyToTextureArray({ rsm_low.GetFramebuffer().getTargetAsTexture(ReflectiveShadowMap::Targets::Depth) },      RSM_LowRes_DepthBuffers_TextureArray);

                pyr::RenderProfiles::popDepthProfile();


            }

            void RenderFn()
            {
                // -- Render all objects 
                for (const StaticMesh* mesh : owner->GetContext().ActorsToRender.meshes)
                {
                    mesh->bindModel();
                    pActorBuffer->setData(ActorBuffer::data_t{ .modelMatrix = mesh->GetTransform().getWorldMatrix() });

                    RSMComputeEffect->bindConstantBuffer("CameraBuffer", pcameraBuffer);
                    RSMComputeEffect->bindConstantBuffer("ActorBuffer", pActorBuffer);
                    RSMComputeEffect->bindConstantBuffer("SingleLightBuffer", pLightBuffer);

                    std::span<const SubMesh> submeshes = mesh->getModel()->getRawMeshData()->getSubmeshes();
                    for (auto& submesh : submeshes)
                    {
                        const auto& submeshMaterial = mesh->getMaterial(submesh.materialIndex);
                        PYR_ENSURE(submeshMaterial, "No material has been found for the submesh. This is unexpected, as default material should still be here !");
                        if (const pyr::Texture* tex = submeshMaterial->getTexture(TextureType::ALBEDO); tex)
                        {
                            RSMComputeEffect->bindTexture(*tex, "albedoTexture");
                        }

                        RSMComputeEffect->setUniform<vec4>("Ka", submeshMaterial->getMaterialRenderingCoefficients().Ka);

                        RSMComputeEffect->bind();
                        Engine::d3dcontext().DrawIndexed(static_cast<UINT>(submesh.getIndexCount()), submesh.startIndex, 0);
                        RSMComputeEffect->unbindResources();
                    }
                }
            }
        };
    }
}
