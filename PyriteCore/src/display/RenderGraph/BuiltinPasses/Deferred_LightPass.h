#pragma once

#include "display/RenderGraph/RenderPass.h"
#include "display/RenderGraph/RenderGraph.h"
#include "display/GraphicalResource.h"
#include "world/Mesh/RawMeshData.h"
#include "world/Mesh/StaticMesh.h"
#include "world/Tools/CommonConstantBuffers.h"
#include "display/FrameBuffer.h"

#include <imgui.h>

namespace pyr
{

    namespace BuiltinPasses
    {

        class Deferred_LightPass : public RenderPass
        {
        private:

            pyr::GraphicalResourceRegistry m_registry;

        private:

            std::shared_ptr<CameraBuffer>    pcameraBuffer = std::make_shared<CameraBuffer>();
            std::shared_ptr<LightsBuffer>    pLightBuffer = std::make_shared<LightsBuffer>();

            Effect* m_lightEffect = nullptr;
            FrameBuffer m_target;

        public:

            Deferred_LightPass(unsigned int width, unsigned int height)
                : m_target{ width , height, pyr::FrameBuffer::COLOR_0 }
            {
                displayName = "Deferred Light Pass";
                m_lightEffect = m_registry.loadEffect(
                    L"res/shaders/deferred_lightpass.fx",
                    InputLayout::MakeLayoutFromVertex<pyr::EmptyVertex>()
                );

                m_target.getTargetAsTexture(pyr::FrameBuffer::COLOR_0).SetDebugName("Deferred_Lightpass");
                producesResource("Deferred_Lightpass", m_target.getTargetAsTexture(pyr::FrameBuffer::COLOR_0));
                
            }

            Deferred_LightPass()
                : Deferred_LightPass(pyr::Device::getWinWidth(), pyr::Device::getWinHeight())
            {}

            virtual void apply() override
            {
                if (!PYR_ENSURE(owner)) return;
                if (!PYR_ENSURE(owner->GetContext().contextCamera)) return;
                pcameraBuffer->setData(CameraBuffer::data_t{ .mvp = owner->GetContext().contextCamera->getViewProjectionMatrix(), .pos = owner->GetContext().contextCamera->getPosition() });

                // -- Fetch the G_Buffer
                pyr::TextureArray G_Buffer = std::get<pyr::TextureArray>(owner->getResourcesManager().fetchResource("G_Buffer"));
                pyr::Texture DepthPrePass = std::get<pyr::Texture>(owner->getResourcesManager().fetchResource("depthBuffer"));

                // -- Fetch the scene lights
                pyr::LightsCollections& lights = owner->GetContext().ActorsToRender.lights;
                LightsBuffer::data_t light_data{};
                std::copy_n(lights.ConvertCollectionToHLSL().begin(), std::size(light_data.lights), std::begin(light_data.lights));
                pLightBuffer->setData(light_data);

                // -- Prepare the framebuffer
                m_target.clearTargets();
                m_target.bind();

                // -- Bind everything to the lightpass shader
                m_lightEffect->bindConstantBuffer("lightsBuffer", pLightBuffer);
                m_lightEffect->bindConstantBuffer("CameraBuffer", pcameraBuffer);
                m_lightEffect->bindTexture(G_Buffer, "G_Buffer");
                m_lightEffect->bindTexture(DepthPrePass, "DepthBuffer");


                // -- Fetch optional resources (SSAO, GI, Shadows...)
                std::optional<NamedResource::resource_t> ssaoTexture = owner->getResourcesManager().fetchOptionalResource("ssaoTexture_blurred");
                std::optional<NamedResource::resource_t> GI_CompositeTexture = owner->getResourcesManager().fetchOptionalResource("GI_CompositeIndirectIllumination");

                if (ssaoTexture) m_lightEffect->bindTexture(std::get<Texture>(ssaoTexture.value()), "ssaoTexture");
                else m_lightEffect->bindTexture(pyr::Texture::getDefaultTextureSet().WhitePixel, "ssaoTexture");

                if (GI_CompositeTexture) m_lightEffect->bindTexture(std::get<Texture>(GI_CompositeTexture.value()), "GI_CompositeTexture");
                else m_lightEffect->bindTexture(pyr::Texture::getDefaultTextureSet().BlackPixel, "GI_CompositeTexture");

                if (auto opt = owner->getResourcesManager().fetchOptionalResource("Lightmaps_2D"); opt.has_value())
                {
                    m_lightEffect->bindTexture(std::get<pyr::TextureArray>(*opt), "lightmaps_2D");
                }
                if (auto opt = owner->getResourcesManager().fetchOptionalResource("Lightmaps_3D"); opt.has_value())
                {
                    m_lightEffect->bindTexture(std::get<pyr::TextureArray>(*opt), "lightmaps_3D");
                }

                // -- Issue blit draw call
                m_lightEffect->bind();
                pyr::Engine::d3dcontext().DrawIndexed(3, 0, 0);
                m_lightEffect->unbindResources();

                m_target.unbind();

            }
        };
    }
}
