#pragma once

#include "imgui.h"
#include "display/DebugDraw.h"
#include "display/IndexBuffer.h"
#include "display/InputLayout.h"
#include "display/Vertex.h"
#include "display/VertexBuffer.h"
#include "display/RenderGraph/RenderGraph.h"
#include "display/RenderGraph/BuiltinPasses/BuiltinPasses.h"
#include "engine/Engine.h"
#include "scene/Scene.h"
#include "world/camera.h"
#include "world/Mesh/MeshImporter.h"
#include "display/GraphicalResource.h"
#include "display/RenderProfiles.h"
#include "world/RayCasting.h"
#include <display/CubemapBuilder.h>

#define IMGUI_DECLARE_FLOAT_UNIFORM(name,shader,a,b) static float name;\
    if (ImGui::SliderFloat(#name, &name, a, b))\
        shader->setUniform<float>(#name, name);    

namespace pye
{

    class CubemapBuilderScene : public pyr::Scene
    {

        std::array<vec3, 6> lookAtDirs{ {
                            {1,0,0},                // right
                            {-1,0,0},               // left   
                            {0,1,0},                // up
                            {0,-1,0},               // down
                            {0,0, 1},               // front
                            {0,0,-1},               // behind
                        } };

    private:
        
        enum RenderMode { SKYBOX, EQUIPROJ, IRRADIANCE, SPECULAR };


        pyr::GraphicalResourceRegistry m_registry;
        pyr::Effect* m_equiproj;
        using CameraBuffer = pyr::ConstantBuffer < InlineStruct(mat4 mvp; alignas(16) vec3 pos) > ;
        std::shared_ptr<CameraBuffer>           pcameraBuffer = std::make_shared<CameraBuffer>();
        pyr::Camera m_camera;
        pyr::Texture m_hdrMap;
        std::array<pyr::FrameBuffer, 6> framebuffers;
        std::array<pyr::FrameBuffer, 6> framebuffers_irradiance;
        bool bSkipImgui = false;
        pyr::Effect* m_skyboxEffect;
        pyr::Effect* m_irradiancePrecompute;
        pyr::Effect* m_specularPreFilter;

        std::shared_ptr<pyr::Cubemap> computedCubemap;
        std::shared_ptr<pyr::Cubemap> computedCubemap_irradiance;
        std::shared_ptr<pyr::Cubemap> computedCubemap_filtered;
        
        pyr::FreecamController m_camController;
        bool bRuntimeFramebufferIrradianceCapture = false;
        bool bRuntimeFramebufferPrefilterCapture = false;
        bool bRenderIrradianceMap = false;
        float m_currentPrefilterRougness = 0.0F;
        RenderMode renderMode = SKYBOX;

    public:

        CubemapBuilderScene()
        {
            m_skyboxEffect              = m_registry.loadEffect(L"res/shaders/skybox.fx", pyr::InputLayout::MakeLayoutFromVertex<pyr::EmptyVertex>());
            m_equiproj                  = m_registry.loadEffect(L"res/shaders/EquirectangularProjection.fx", pyr::InputLayout::MakeLayoutFromVertex<pyr::RawMeshData::mesh_vertex_t>());
            m_irradiancePrecompute      = m_registry.loadEffect(L"res/shaders/irradiancePreCompute.fx", pyr::InputLayout::MakeLayoutFromVertex<pyr::EmptyVertex>());
            m_specularPreFilter         = m_registry.loadEffect(L"res/shaders/specularIBL_mips.fx", pyr::InputLayout::MakeLayoutFromVertex<pyr::EmptyVertex>());

            m_hdrMap = m_registry.loadTexture(L"textures/HDR/2.hdr");

            m_camera.setProjection(pyr::PerspectiveProjection{ .fovy = 3.141592f/2.f, .aspect = 1.F} ); // this gives an FOVx of 90 in radians...
            m_camController.setCamera(&m_camera);
        }

        void takePicturesOfSurroundings()
        {
            bSkipImgui = true;

            //=============================================================================//
            // Diffuse irradiance
            // -- Convert the equiproj map to a cubemap
            renderMode = RenderMode::EQUIPROJ;
            auto& device = pyr::Engine::device();
            for (int i = 0; i < 6; i++)
            {
                m_camera.lookAt(lookAtDirs[i]);
                if (i == 2) m_camera.rotate(0, 3.14159, 0); // why ? it works
                if (i == 3) m_camera.rotate(0, 3.14159, 0); // 
                pcameraBuffer->setData(CameraBuffer::data_t{
                    .mvp = m_camera.getViewProjectionMatrix(),
                    .pos = m_camera.getPosition()
                    });
                framebuffers[i] = pyr::FrameBuffer{ 1024, 1024, pyr::FrameBuffer::COLOR_0 };
                framebuffers[i].bind();
                render();
                framebuffers[i].unbind();
            }
            std::array<pyr::Texture, 6> textures = {
                framebuffers[0].getTargetAsTexture(pyr::FrameBuffer::COLOR_0),
                framebuffers[1].getTargetAsTexture(pyr::FrameBuffer::COLOR_0),
                framebuffers[2].getTargetAsTexture(pyr::FrameBuffer::COLOR_0),
                framebuffers[3].getTargetAsTexture(pyr::FrameBuffer::COLOR_0),
                framebuffers[4].getTargetAsTexture(pyr::FrameBuffer::COLOR_0),
                framebuffers[5].getTargetAsTexture(pyr::FrameBuffer::COLOR_0)
            };
            computedCubemap = std::make_shared<pyr::Cubemap>(pyr::CubemapBuilder::MakeCubemapFromTextures(textures));

            // -- Convert the HDR cubemap to a precomputed irradiance cubemap
            pcameraBuffer->setData(CameraBuffer::data_t{
                    .mvp = m_camera.getViewProjectionMatrix(),
                    .pos = m_camera.getPosition()
                });
            renderMode = RenderMode::IRRADIANCE;
            for (int i = 0; i < 6; i++)
            {
                m_camera.lookAt(lookAtDirs[i]);
                if (i == 2) m_camera.rotate(0, 3.14159, 0); 
                if (i == 3) m_camera.rotate(0, 3.14159, 0);  
                pcameraBuffer->setData(CameraBuffer::data_t{
                    .mvp = m_camera.getViewProjectionMatrix(),
                    .pos = m_camera.getPosition()
                });
                framebuffers[i] = pyr::FrameBuffer{ 32, 32, pyr::FrameBuffer::COLOR_0 };
                framebuffers[i].bind();
                render();
                framebuffers[i].unbind();
            }
            std::array<pyr::Texture, 6> textures2 = {
                framebuffers[0].getTargetAsTexture(pyr::FrameBuffer::COLOR_0),
                framebuffers[1].getTargetAsTexture(pyr::FrameBuffer::COLOR_0),
                framebuffers[2].getTargetAsTexture(pyr::FrameBuffer::COLOR_0),
                framebuffers[3].getTargetAsTexture(pyr::FrameBuffer::COLOR_0),
                framebuffers[4].getTargetAsTexture(pyr::FrameBuffer::COLOR_0),
                framebuffers[5].getTargetAsTexture(pyr::FrameBuffer::COLOR_0)
            };
            computedCubemap_irradiance = std::make_shared<pyr::Cubemap>(pyr::CubemapBuilder::MakeCubemapFromTextures(textures2));

            //=============================================================================//
            // Specular pre-filter
            renderMode = RenderMode::SPECULAR;
            constexpr size_t mipCount = 5;
            uint32_t baseWidth = 256;
            std::array<pyr::Texture, mipCount * 6> prefiltered;
            for (size_t mipLevel = 0; mipLevel < mipCount; mipLevel++)
            {
                m_currentPrefilterRougness = (float)mipLevel / (float)mipCount;
                // Fill the 6 framebuffers
                uint32_t framebufferWidth = baseWidth / std::pow(2, mipLevel);
                for (int i = 0; i < 6; i++)
                {
                    m_camera.lookAt(lookAtDirs[i]);
                    if (i == 2) m_camera.rotate(0, 3.14159, 0);
                    if (i == 3) m_camera.rotate(0, 3.14159, 0);
                    pcameraBuffer->setData(CameraBuffer::data_t{
                        .mvp = m_camera.getViewProjectionMatrix(),
                        .pos = m_camera.getPosition()
                        });
                    framebuffers[i] = pyr::FrameBuffer{ framebufferWidth, framebufferWidth, pyr::FrameBuffer::COLOR_0 };
                    framebuffers[i].bind();
                    render();
                    framebuffers[i].unbind();
                    prefiltered[mipLevel * 6 + i] = framebuffers[i].getTargetAsTexture(pyr::FrameBuffer::COLOR_0);
                    m_registry.keepHandleToTexture(prefiltered[mipLevel * 6 + i]);

                }
            }

            computedCubemap_filtered = std::make_shared<pyr::Cubemap>(pyr::CubemapBuilder::MakeCubemapFromTexturesLOD<mipCount>(prefiltered));

            // -- Reset render mode
            bSkipImgui = false;
            renderMode = RenderMode::SKYBOX;
        }

        void update(float delta) override
        {
            m_camController.processUserInputs(delta);
            pcameraBuffer->setData(CameraBuffer::data_t{
                .mvp = m_camera.getViewProjectionMatrix(),
                .pos = m_camera.getPosition()
            });
        }


        void render() override

        {
            // -- debug
            constexpr size_t mipCount = 5;
            uint32_t baseWidth = 256;
            static std::array<pyr::Texture, mipCount * 6> prefiltered;

            pyr::Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            pyr::RenderProfiles::pushRasterProfile(pyr::RasterizerProfile::NOCULL_RASTERIZER);
            pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTWRITE_DEPTH);


            switch (renderMode)
            {
                case SKYBOX:
                {
                    if (!computedCubemap_irradiance || !computedCubemap) break;
                    m_skyboxEffect->bindConstantBuffer("CameraBuffer", pcameraBuffer);
                    m_skyboxEffect->bindCubemap(bRenderIrradianceMap ? *computedCubemap_irradiance : *computedCubemap, "cubemap");
                    m_skyboxEffect->bind();
                    break;
                }

                case EQUIPROJ:
                {
                    m_equiproj->bindConstantBuffer("CameraBuffer", pcameraBuffer);
                    m_equiproj->bindTexture(m_hdrMap, "mat_hdr");
                    m_equiproj->bind();
                    break;
                }

                case IRRADIANCE:
                {
                    if (!computedCubemap) break;

                    if (bRuntimeFramebufferIrradianceCapture)
                    {
                        for (int i = 0; i < 6; i++)
                        {
                            m_camera.lookAt(lookAtDirs[i]);
                            if (i == 2) m_camera.rotate(0, 3.14159f, 0);
                            if (i == 3) m_camera.rotate(0, 3.14159f, 0);
                            update(0.0F);
                            framebuffers[i] = pyr::FrameBuffer{ 32, 32, pyr::FrameBuffer::COLOR_0 };
                            framebuffers[i].bind();
                            m_irradiancePrecompute->bind();
                            m_irradiancePrecompute->bindConstantBuffer("CameraBuffer", pcameraBuffer);
                            m_irradiancePrecompute->bindCubemap(*computedCubemap, "cubemapHDR");
                            pyr::Engine::d3dcontext().Draw(36, 0);
                            framebuffers[i].unbind();
                        }
                        bRuntimeFramebufferIrradianceCapture = false;
                        renderMode = SKYBOX;
                    }
                    else {

                        m_irradiancePrecompute->bindConstantBuffer("CameraBuffer", pcameraBuffer);
                        m_irradiancePrecompute->bindCubemap(*computedCubemap, "cubemapHDR");
                        m_irradiancePrecompute->bind();
                    }
                    break;
                }

                case SPECULAR:
                {

                    if (bRuntimeFramebufferPrefilterCapture && computedCubemap)
                    {
                        for (size_t mipLevel = 0; mipLevel < mipCount; mipLevel++)
                        {
                            m_currentPrefilterRougness = (float)mipLevel / (float)mipCount;
                            uint32_t framebufferWidth = baseWidth / std::pow(2, mipLevel);
                            for (int i = 0; i < 6; i++)
                            {
                                m_camera.lookAt(lookAtDirs[i]);
                                if (i == 2) m_camera.rotate(0, 3.14159, 0);
                                if (i == 3) m_camera.rotate(0, 3.14159, 0);
                                pcameraBuffer->setData(CameraBuffer::data_t{
                                    .mvp = m_camera.getViewProjectionMatrix(),
                                    .pos = m_camera.getPosition()
                                    });
                                framebuffers[i] = pyr::FrameBuffer{ framebufferWidth, framebufferWidth, pyr::FrameBuffer::COLOR_0 };
                                framebuffers[i].bind();
                                
                                m_specularPreFilter->bindConstantBuffer("CameraBuffer", pcameraBuffer);
                                m_specularPreFilter->setUniform<float>("roughness", m_currentPrefilterRougness);
                                m_specularPreFilter->bindCubemap(*computedCubemap, "environmentMap");
                                m_specularPreFilter->bind();
                                pyr::Engine::d3dcontext().Draw(36, 0);

                                framebuffers[i].unbind();
                                prefiltered[mipLevel * 6 + i] = framebuffers[i].getTargetAsTexture(pyr::FrameBuffer::COLOR_0);

                            }
                        }
                    }
                    else
                    {
                        if (!computedCubemap) break;
                        m_specularPreFilter->bindConstantBuffer("CameraBuffer", pcameraBuffer);
                        m_specularPreFilter->setUniform<float>("roughness", m_currentPrefilterRougness);
                        m_specularPreFilter->bindCubemap(*computedCubemap, "environmentMap");
                        m_specularPreFilter->bind();
                    }
                    break;
                }
                default:
                    break;


            }
            pyr::Engine::d3dcontext().Draw(36, 0);
            pyr::Effect::unbindResources();
            pyr::RenderProfiles::popDepthProfile();
            pyr::RenderProfiles::popRasterProfile();

            if (bSkipImgui) return;
            ImGui::Begin("Cubemap Processing");
            if (ImGui::Button("Produce cubemap"))
            {
                takePicturesOfSurroundings();
            }
            if (bRuntimeFramebufferIrradianceCapture && renderMode == RenderMode::IRRADIANCE && ImGui::Button("Recapture irradiance cubemap"))
            {
                std::array<pyr::Texture, 6> textures2 = {
                    framebuffers[0].getTargetAsTexture(pyr::FrameBuffer::COLOR_0),
                    framebuffers[1].getTargetAsTexture(pyr::FrameBuffer::COLOR_0),
                    framebuffers[2].getTargetAsTexture(pyr::FrameBuffer::COLOR_0),
                    framebuffers[3].getTargetAsTexture(pyr::FrameBuffer::COLOR_0),
                    framebuffers[4].getTargetAsTexture(pyr::FrameBuffer::COLOR_0),
                    framebuffers[5].getTargetAsTexture(pyr::FrameBuffer::COLOR_0)
                };
                computedCubemap_irradiance = std::make_unique<pyr::Cubemap>(pyr::CubemapBuilder::MakeCubemapFromTextures(textures2));
                
                bRuntimeFramebufferIrradianceCapture = false;

            }

            if (bRuntimeFramebufferPrefilterCapture && renderMode == RenderMode::SPECULAR)
            {
                computedCubemap_filtered = std::make_shared<pyr::Cubemap>(pyr::CubemapBuilder::MakeCubemapFromTexturesLOD<mipCount>(prefiltered));
                bRuntimeFramebufferPrefilterCapture = false;
            }

            ImGui::Checkbox("Skybox render using irradiance compute :", &bRenderIrradianceMap);
            ImGui::Checkbox("Prefilter runtime", &bRuntimeFramebufferPrefilterCapture);
            if (ImGui::Button("ReCompute irradiance cubemapat runtime :"))
            {
                renderMode = IRRADIANCE;
                bRuntimeFramebufferIrradianceCapture = true;
            }
            static const char* items[]{ "Skybox","Equirectangular projection","Irradiance compute", "Specular pre-filter"};
            static int Selecteditem = 0;
            if (ImGui::Combo("Skybox Render Mode", &Selecteditem, items, IM_ARRAYSIZE(items)))
            {
                renderMode = static_cast<RenderMode>(Selecteditem);
            }

            ImGui::End();

        }
    };
}