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
#include "RenderDoc/renderdoc_app.h"
#include <display/CubemapBuilder.h>
#include "world/Tools/CommonConstantBuffers.h"

#include "RenderDoc/renderdoc_app.h"

RENDERDOC_API_1_1_2* rdoc_api = NULL;

namespace pye
{
    struct ComputedCubemapCollection
    {
        /* The cubemap computed from the 2D HDR file */
        std::shared_ptr<pyr::Cubemap> Cubemap;       

        /* Blurred version of the cubemap used to compute ambient lighting */
        std::shared_ptr<pyr::Cubemap> Irradiance;

        /* A Cubemap with multiple mips depending on the roughness, used for reflections / specular lighting */
        std::shared_ptr<pyr::Cubemap> SpecularFiltered;
    };
    // This scene is an actual mess
    class CubemapBuilderScene : public pyr::Scene
    {
        std::array<vec3, 6> lookAtDirs{ {
                            {1,0,0},                // right
                            {-1,0,0},               // left   
                            {0,1,0},                // up
                            {0,-1,0},               // down
                            {0,0, 1},               // front
                            {0,0,-1},               // behind
        }};

    private:
        
        enum RenderMode { SKYBOX, EQUIPROJ, IRRADIANCE, SPECULAR };
        RenderMode renderMode = SKYBOX;

        pyr::GraphicalResourceRegistry m_registry;
        std::shared_ptr<pyr::CameraBuffer>  pcameraBuffer = std::make_shared<pyr::CameraBuffer>();

        pyr::Camera m_camera;
        pyr::Texture m_hdrMap;
        std::array<pyr::FrameBuffer, 6> framebuffers;
        
        pyr::Effect* m_equiproj;
        pyr::Effect* m_skyboxEffect;
        pyr::Effect* m_irradiancePrecompute;
        pyr::Effect* m_specularPreFilter;
        pyr::Effect* m_specularBRDF;
        
        pyr::FreecamController m_camController;
        
        float m_currentPrefilterRougness = 0.0F;
        bool bRuntimeFramebufferIrradianceCapture = false;
        bool bRuntimeFramebufferPrefilterCapture = false;
        bool bRenderIrradianceMap = false;
        bool bSkipImgui = false;


    public:

        pyr::Texture BRDF_Lut = m_registry.loadTexture(L"res/textures/pbr/brdfLUT.png"); // builtin BRDF LUT (the shader does not work and i dont care about it)
        ComputedCubemapCollection OutputCubemaps;
        std::filesystem::path DefaultHDRMap = L"res/textures/pbr/fireplace_4k.hdr";


        CubemapBuilderScene()
        {

            m_hdrMap                    = m_registry.loadTexture(DefaultHDRMap);
            m_skyboxEffect              = m_registry.loadEffect(L"res/shaders/skybox.fx", pyr::InputLayout::MakeLayoutFromVertex<pyr::EmptyVertex>());
            m_equiproj                  = m_registry.loadEffect(L"res/shaders/EquirectangularProjection.fx", pyr::InputLayout::MakeLayoutFromVertex<pyr::RawMeshData::mesh_vertex_t>());
            m_irradiancePrecompute      = m_registry.loadEffect(L"res/shaders/irradiancePreCompute.fx", pyr::InputLayout::MakeLayoutFromVertex<pyr::EmptyVertex>());
            m_specularPreFilter         = m_registry.loadEffect(L"res/shaders/specularIBL_mips.fx", pyr::InputLayout::MakeLayoutFromVertex<pyr::EmptyVertex>());
            m_specularBRDF              = m_registry.loadEffect(L"res/shaders/specularIBL_BRDFPrecompute.fx", pyr::InputLayout::MakeLayoutFromVertex<pyr::EmptyVertex>());


            m_camera.setProjection(pyr::PerspectiveProjection{ .fovy = 3.141592f/2.f, .aspect = 1.F} ); // this gives an FOVx of 90 in radians...
            m_camController.setCamera(&m_camera);
        }

        void SetHDRBackground(const std::filesystem::path& path)
        {
            m_hdrMap = m_registry.loadTexture(path);
        }

        void ComputeIBLCubemaps()
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
                if (i == 2) m_camera.rotate(0.f, 3.14159f, 0.f); // why ? it works
                if (i == 3) m_camera.rotate(0.f, 3.14159f, 0.f); // 
                pcameraBuffer->setData(pyr::CameraBuffer::data_t{
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
            OutputCubemaps.Cubemap = std::make_shared<pyr::Cubemap>(pyr::CubemapBuilder::MakeCubemapFromTextures(textures));

            // -- Convert the HDR cubemap to a precomputed irradiance cubemap
            pcameraBuffer->setData(pyr::CameraBuffer::data_t{
                    .mvp = m_camera.getViewProjectionMatrix(),
                    .pos = m_camera.getPosition()
                });
            renderMode = RenderMode::IRRADIANCE;
            for (int i = 0; i < 6; i++)
            {
                m_camera.lookAt(lookAtDirs[i]);
                if (i == 2) m_camera.rotate(0.f, 3.14159f, 0.f); 
                if (i == 3) m_camera.rotate(0.f, 3.14159f, 0.f);
                pcameraBuffer->setData(pyr::CameraBuffer::data_t{
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
            OutputCubemaps.Irradiance = std::make_shared<pyr::Cubemap>(pyr::CubemapBuilder::MakeCubemapFromTextures(textures2));

            //=============================================================================//
            // Specular pre-filter
            renderMode = RenderMode::SPECULAR;
            static constexpr size_t REFLECTION_RESOLUTION = 2048;

            constexpr size_t mipCount = 5;
            uint32_t baseWidth = REFLECTION_RESOLUTION;
            std::array<pyr::Texture, mipCount * 6> prefiltered;
            for (size_t mipLevel = 0; mipLevel < mipCount; mipLevel++)
            {
                m_currentPrefilterRougness = (float)mipLevel / (float)(mipCount);
                // Fill the 6 framebuffers
                uint32_t framebufferWidth = baseWidth / static_cast<uint32_t>(std::pow(2, mipLevel));
                for (int i = 0; i < 6; i++)
                {
                    m_camera.lookAt(lookAtDirs[i]);
                    if (i == 2) m_camera.rotate(0.f, 3.14159f, 0.f);
                    if (i == 3) m_camera.rotate(0.f, 3.14159f, 0.f);
                    pcameraBuffer->setData(pyr::CameraBuffer::data_t{
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

            OutputCubemaps.SpecularFiltered = std::make_shared<pyr::Cubemap>(pyr::CubemapBuilder::MakeCubemapFromTexturesLOD<mipCount>(prefiltered));

            //=============================================================================//
            // Specular brdf
            framebuffers[0] = pyr::FrameBuffer{ REFLECTION_RESOLUTION, REFLECTION_RESOLUTION, pyr::FrameBuffer::COLOR_0 };
            framebuffers[0].bind();
            m_specularBRDF->bind();
            pyr::Engine::d3dcontext().Draw(6, 0);
            m_specularBRDF->unbindResources();
            framebuffers[0].unbind();

            m_registry.keepHandleToTexture(framebuffers[0].getTargetAsTexture(pyr::FrameBuffer::COLOR_0));
            BRDF_Lut = framebuffers[0].getTargetAsTexture(pyr::FrameBuffer::COLOR_0);
            //=============================================================================//
            // -- Reset render mode
            bSkipImgui = false;
            renderMode = RenderMode::SKYBOX;
        }

        void update(float delta) override
        {
            m_camController.processUserInputs(delta);
            pcameraBuffer->setData(pyr::CameraBuffer::data_t{
                .mvp = m_camera.getViewProjectionMatrix(),
                .pos = m_camera.getPosition()
            });
        }


        void render() override

        {
            // -- Debug runtime values, should not be used if you are not in the scene
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
                    if (!OutputCubemaps.Irradiance || !OutputCubemaps.Cubemap) break;
                    m_skyboxEffect->bindConstantBuffer("CameraBuffer", pcameraBuffer);
                    m_skyboxEffect->bindConstantBuffer("CameraBuffer", pcameraBuffer);
                    if (OutputCubemaps.SpecularFiltered)
                    {
                        m_skyboxEffect->bindCubemap(*OutputCubemaps.SpecularFiltered, "cubemap");
                    }
                    else
                    {
                        m_skyboxEffect->bindCubemap(bRenderIrradianceMap ? *OutputCubemaps.Irradiance : *OutputCubemaps.Cubemap, "cubemap");
                    }
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
                    if (!OutputCubemaps.Cubemap) break;

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
                            m_irradiancePrecompute->bindCubemap(*OutputCubemaps.Cubemap, "cubemapHDR");
                            pyr::Engine::d3dcontext().Draw(36, 0);
                            framebuffers[i].unbind();
                        }
                        bRuntimeFramebufferIrradianceCapture = false;
                        renderMode = SKYBOX;
                    }
                    else {

                        m_irradiancePrecompute->bindConstantBuffer("CameraBuffer", pcameraBuffer);
                        m_irradiancePrecompute->bindCubemap(*OutputCubemaps.Cubemap, "cubemapHDR");
                        m_irradiancePrecompute->bind();
                    }
                    break;
                }

                case SPECULAR:
                {

                    if (bRuntimeFramebufferPrefilterCapture && OutputCubemaps.Cubemap)
                    {
                        for (size_t mipLevel = 0; mipLevel < mipCount; mipLevel++)
                        {
                            m_currentPrefilterRougness = (float)mipLevel / (float)mipCount;
                            uint32_t framebufferWidth = baseWidth / static_cast<uint32_t>(std::pow(2, mipLevel));
                            for (int i = 0; i < 6; i++)
                            {
                                m_camera.lookAt(lookAtDirs[i]);
                                if (i == 2) m_camera.rotate(0, 3.14159f, 0);
                                if (i == 3) m_camera.rotate(0, 3.14159f, 0);
                                pcameraBuffer->setData(pyr::CameraBuffer::data_t{
                                    .mvp = m_camera.getViewProjectionMatrix(),
                                    .pos = m_camera.getPosition()
                                    });
                                framebuffers[i] = pyr::FrameBuffer{ framebufferWidth, framebufferWidth, pyr::FrameBuffer::COLOR_0 };
                                framebuffers[i].bind();
                                
                                m_specularPreFilter->bindConstantBuffer("CameraBuffer", pcameraBuffer);
                                m_specularPreFilter->setUniform<float>("roughness", m_currentPrefilterRougness);
                                m_specularPreFilter->bindCubemap(*OutputCubemaps.Cubemap, "environmentMap");
                                m_specularPreFilter->bind();
                                pyr::Engine::d3dcontext().Draw(36, 0);

                                framebuffers[i].unbind();
                                prefiltered[mipLevel * 6 + i] = framebuffers[i].getTargetAsTexture(pyr::FrameBuffer::COLOR_0);

                            }
                        }
                    }
                    else
                    {
                        if (!OutputCubemaps.Cubemap) break;
                        m_specularPreFilter->bindConstantBuffer("CameraBuffer", pcameraBuffer);
                        m_specularPreFilter->setUniform<float>("roughness", m_currentPrefilterRougness);
                        m_specularPreFilter->bindCubemap(*OutputCubemaps.Cubemap, "environmentMap");
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

            // -- Debug stuff if you need to be in the scene
            if (bSkipImgui) return;
            ImGui::Begin("Cubemap Processing");
            if (ImGui::Button("Produce cubemap"))
            {
                ComputeIBLCubemaps();
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
                OutputCubemaps.Irradiance = std::make_unique<pyr::Cubemap>(pyr::CubemapBuilder::MakeCubemapFromTextures(textures2));
                
                bRuntimeFramebufferIrradianceCapture = false;

            }

            if (bRuntimeFramebufferPrefilterCapture && renderMode == RenderMode::SPECULAR)
            {
                OutputCubemaps.SpecularFiltered = std::make_shared<pyr::Cubemap>(pyr::CubemapBuilder::MakeCubemapFromTexturesLOD<mipCount>(prefiltered));
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