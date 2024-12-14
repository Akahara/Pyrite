#pragma once

#include "display/RenderGraph/RenderPass.h"
#include "display/RenderGraph/RenderGraph.h"
#include "display/GraphicalResource.h"
#include "display/FrameBuffer.h"
#include "world/Mesh/RawMeshData.h"
#include "world/camera.h"
#include "display/RenderProfiles.h"
#include "world/Mesh/StaticMesh.h"
#include "world/Lights/Light.h"
#include "world/Shadows/Lightmap.h"
#include "world/Tools/SceneRenderTools.h"
#include "scene/SceneManager.h"

#include <array>

namespace pyr
{

    namespace BuiltinPasses
    {
        class ForwardPass : public RenderPass
        {
            private:

                GraphicalResourceRegistry m_registry;

                std::shared_ptr<ActorBuffer>     pActorBuffer  = std::make_shared<ActorBuffer>();
                std::shared_ptr<CameraBuffer>    pcameraBuffer = std::make_shared<CameraBuffer>();
                std::shared_ptr<LightsBuffer>    pLightBuffer  = std::make_shared<LightsBuffer>();
    
            public:

                ForwardPass()
                {
                    displayName = "Forward pass";
                }

                virtual void apply() override
                {
                    PYR_ENSURE(owner);
                    if (!PYR_ENSURE(owner->GetContext().contextCamera)) return;
                    pcameraBuffer->setData(CameraBuffer::data_t{ .mvp = owner->GetContext().contextCamera->getViewProjectionMatrix(), .pos = owner->GetContext().contextCamera->getPosition() });
                    
                    Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                    pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTONLY_DEPTH);
                    
                    Texture depthPrePassBuffer = std::get<Texture>(owner->getResourcesManager().fetchResource("depthBuffer"));
                    pyr::FrameBuffer::getActiveFrameBuffer().setDepthOverride(depthPrePassBuffer.toDepthStencilView()); // < make sure this input is linked in the scene rdg
                    
                    // -- Create raw light buffer
                    pyr::LightsCollections& lights = owner->GetContext().ActorsToRender.lights;
                    LightsBuffer::data_t light_data{};
                    std::copy_n(lights.ConvertCollectionToHLSL().begin(), std::size(light_data.lights), std::begin(light_data.lights));
                    pLightBuffer->setData(light_data);
                    
                    std::optional<NamedResource::resource_t> GI_CompositeTexture = owner->getResourcesManager().fetchOptionalResource("GI_CompositeIndirectIllumination");
                    
                    // -- Render all objects 
                    for (const StaticMesh* mesh : owner->GetContext().ActorsToRender.meshes)
                    {
                        mesh->bindModel();
                        pActorBuffer->setData(ActorBuffer::data_t{ .modelMatrix = mesh->GetTransform().getWorldMatrix() });
                        std::span<const SubMesh> submeshes = mesh->getModel()->getRawMeshData()->getSubmeshes();
                    
                        std::optional<NamedResource::resource_t> ssaoTexture = owner->getResourcesManager().fetchOptionalResource("ssaoTexture_blurred");
                    
                        for (auto& submesh : submeshes)
                        {
                            //const auto submeshMaterial = pyr::MaterialBank::GetMaterialReference(submesh.materialIndex);
                            const auto& submeshMaterial = mesh->getMaterial(submesh.materialIndex);
                            if (!submeshMaterial) break; // should not happen because of default mat ?
                      
                            const Effect* effect = submeshMaterial->getEffect();
                            if (!effect) break;
                      
                            effect->bindConstantBuffer("CameraBuffer", pcameraBuffer);
                            effect->bindConstantBuffer("ActorBuffer", pActorBuffer);
                            effect->bindConstantBuffer("ActorMaterials", submeshMaterial->coefsToCbuffer());
                            effect->bindConstantBuffer("lightsBuffer", pLightBuffer);
                      
                            if (auto opt = owner->getResourcesManager().fetchOptionalResource("Lightmaps_2D"); opt.has_value())
                            {
                                effect->bindTexture(std::get<pyr::TextureArray>(*opt), "lightmaps_2D");
                            }
                            if (auto opt = owner->getResourcesManager().fetchOptionalResource("Lightmaps_3D"); opt.has_value())
                            {
                                effect->bindTexture(std::get<pyr::TextureArray>(*opt), "lightmaps_3D");
                            }
                      
                            if (ssaoTexture) effect->bindTexture(std::get<Texture>(ssaoTexture.value()), "ssaoTexture");
                            else effect->bindTexture(pyr::Texture::getDefaultTextureSet().WhitePixel , "ssaoTexture");
                      
                            if (GI_CompositeTexture) effect->bindTexture(std::get<Texture>(GI_CompositeTexture.value()), "GI_CompositeTexture");
                            else effect->bindTexture(pyr::Texture::getDefaultTextureSet().BlackPixel, "GI_CompositeTexture");
                      
                            if (submeshMaterial)
                            {
                                if (auto tex = submeshMaterial->getTexture(TextureType::ALBEDO); tex)    effect->bindTexture(*tex, "mat_albedo");
                                if (auto tex = submeshMaterial->getTexture(TextureType::NORMAL); tex)    effect->bindTexture(*tex, "mat_normal");
                                if (auto tex = submeshMaterial->getTexture(TextureType::BUMP); tex)      effect->bindTexture(*tex, "mat_normal");
                                if (auto tex = submeshMaterial->getTexture(TextureType::AO); tex)        effect->bindTexture(*tex, "mat_ao");
                                if (auto tex = submeshMaterial->getTexture(TextureType::ROUGHNESS); tex) effect->bindTexture(*tex, "mat_roughness");
                                if (auto tex = submeshMaterial->getTexture(TextureType::METALNESS); tex) effect->bindTexture(*tex, "mat_metalness");
                                if (auto tex = submeshMaterial->getTexture(TextureType::HEIGHT); tex)    effect->bindTexture(*tex,  "mat_height");
                            }
                      
                            effect->bind();
                            Engine::d3dcontext().DrawIndexed(static_cast<UINT>(submesh.getIndexCount()), submesh.startIndex, 0);
                            effect->unbindResources();
                        }
                    }
                    
                    pyr::RenderProfiles::popDepthProfile();
                }
    
            };
        }   
}
