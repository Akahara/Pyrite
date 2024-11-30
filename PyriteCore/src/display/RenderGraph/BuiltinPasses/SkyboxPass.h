#pragma once

#include "display/RenderGraph/RenderPass.h"
#include "display/RenderGraph/RenderGraph.h"
#include "display/GraphicalResource.h"
#include "display/FrameBuffer.h"
#include "world/Tools/CommonConstantBuffers.h"
#include "display/RenderProfiles.h"


namespace pyr
{

    namespace BuiltinPasses
    {
    
        class SkyboxPass : public RenderPass
        {
        private:
    
            pyr::GraphicalResourceRegistry m_registry;
            
            std::shared_ptr<pyr::CameraBuffer>  pcameraBuffer = std::make_shared<pyr::CameraBuffer>();
            
            Effect* m_skyboxEffect = nullptr;
            pyr::Cubemap m_currentSkybox;
    
        public:
    
            SkyboxPass()
            {
                displayName = "Skybox Pass";
                m_skyboxEffect = m_registry.loadEffect(L"res/shaders/skybox.fx", InputLayout::MakeLayoutFromVertex<EmptyVertex>());
                m_currentSkybox = m_registry.loadCubemap(L"res/textures/pbr/testhdr.dds");
            }
            
            void SetSkybox(const pyr::Cubemap& skybox)
            {
                m_currentSkybox = skybox;
            }
            
            virtual void apply() override
            {
                //if (!PYR_ENSURE(owner->GetContext().contextCamera)) return;
                pcameraBuffer->setData(CameraBuffer::data_t{ .mvp = owner->GetContext().contextCamera->getViewProjectionMatrix(), .pos = owner->GetContext().contextCamera->getPosition() });
                
                Texture depthPrePassBuffer = std::get<Texture>(owner->getResourcesManager().fetchResource("depthBuffer"));
                pyr::FrameBuffer::getActiveFrameBuffer().setDepthOverride(depthPrePassBuffer.toDepthStencilView()); 
                
                m_skyboxEffect->bindConstantBuffer("CameraBuffer", pcameraBuffer);
                m_skyboxEffect->bindCubemap(m_currentSkybox, "cubemap");
                
                pyr::RenderProfiles::pushRasterProfile(RasterizerProfile::NOCULL_RASTERIZER);
                pyr::RenderProfiles::pushDepthProfile(DepthProfile::TESTWRITE_DEPTH);
                
                m_skyboxEffect->bind();
                Engine::d3dcontext().Draw(36, 0);
                m_skyboxEffect->unbindResources();
                
                RenderProfiles::popDepthProfile();
                RenderProfiles::popRasterProfile();
                
                pyr::FrameBuffer::getActiveFrameBuffer().setDepthOverride(nullptr);
            }
    
        };
    }
}
