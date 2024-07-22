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
#include "CubemapBuilderScene.h"
#include "world/camera.h"
#include "world/Mesh/MeshImporter.h"
#include "display/GraphicalResource.h"
#include "display/RenderProfiles.h"
#include "world/RayCasting.h"

#define IMGUI_DECLARE_FLOAT_UNIFORM(name,shader,a,b) static float name;\
    if (ImGui::SliderFloat(#name, &name, a, b))\
        shader->setUniform<float>(#name, name);    

namespace pye
{

    class MaterialScene : public pyr::Scene
    {
    private:

        pyr::GraphicalResourceRegistry m_registry;
        pyr::Effect* m_ggxShader;
        pyr::Effect* m_equiproj;

        std::shared_ptr<pyr::Model> m_ballModel = pyr::MeshImporter::ImportMeshesFromFile(L"res/meshes/boule.obj").at(0);
        std::shared_ptr<pyr::Model> m_cubeModel = pyr::MeshImporter::ImportMeshesFromFile(L"res/meshes/cube.obj").at(0);
        std::vector<pyr::StaticMesh> m_balls;

        using CameraBuffer = pyr::ConstantBuffer < InlineStruct(mat4 mvp; alignas(16) vec3 pos) > ;
        std::shared_ptr<CameraBuffer>           pcameraBuffer = std::make_shared<CameraBuffer>();
        pyr::BuiltinPasses::ForwardPass m_forwardPass;
        pyr::BuiltinPasses::SSAOPass m_SSAOPass;
        pyr::BuiltinPasses::DepthPrePass m_depthPrePass;
        pyr::RenderGraph m_RDG;

        pyr::Camera m_camera;
        pyr::FreecamController m_camController;
        using InverseCameraBuffer = pyr::ConstantBuffer < InlineStruct(mat4 inverseViewProj;  mat4 inverseProj; alignas(16) mat4 Proj) > ;
        std::shared_ptr<InverseCameraBuffer>    pinvCameBuffer = std::make_shared<InverseCameraBuffer>();
        
        pyr::Texture brdfLUT;

        std::shared_ptr<pyr::Cubemap> specularCubemap;
        std::shared_ptr<pyr::Cubemap> m_irradianceMap;

    public:

        MaterialScene()
        {

            CubemapBuilderScene* cubemapScene = new CubemapBuilderScene();
            cubemapScene->hdrMapPath = L"res/textures/pbr/hdr2.hdr";
            cubemapScene->takePicturesOfSurroundings();
            specularCubemap = cubemapScene->computedCubemap_specularFiltered;
            m_irradianceMap = cubemapScene->computedCubemap_irradiance;
            m_registry.keepHandleToCubemap(*cubemapScene->computedCubemap);
            m_forwardPass.m_skybox = *cubemapScene->computedCubemap;
            brdfLUT = cubemapScene->computed_BRDF;
            delete cubemapScene;

            m_ggxShader = m_registry.loadEffect(L"res/shaders/ggx.fx", pyr::InputLayout::MakeLayoutFromVertex<pyr::RawMeshData::mesh_vertex_t>());
            brdfLUT = m_registry.loadTexture(L"res/textures/pbr/brdfLUT.png");
            m_ggxShader->bindTexture(brdfLUT, "brdfLUT");
            m_ggxShader->bindCubemap(*m_irradianceMap, "irrandiance_map");
            m_ggxShader->bindCubemap(*specularCubemap, "prefilterMap");

#pragma region BALLS
            int gridSize = 7;
            m_balls.resize(gridSize * gridSize, pyr::StaticMesh{ m_ballModel });
            for (int i = 0; i < m_balls.size(); i++)
            {
                m_balls[i].getTransform().position = { (i % gridSize) * 2.f , (i / gridSize) * 2.f ,0};
                
                pyr::MaterialRenderingCoefficients coefs;
                coefs.Ka = { 1,0,1 };
                coefs.Metallic = (i % gridSize) / (gridSize - 1.f);
                coefs.Roughness = std::clamp((i / gridSize) / (gridSize - 1.f), 0.05f, 1.f);
                auto mat = pyr::Material::MakeRegisteredMaterial({}, coefs, m_ggxShader, std::format("Material_%d",i));
                m_balls[i].overrideSubmeshMaterial(0, mat);
                m_forwardPass.addMeshToPass(&m_balls[i]);
                m_depthPrePass.addMeshToPass(&m_balls[i]);
            }
#pragma endregion BALLS

#pragma region RDG
            m_RDG.addPass(&m_forwardPass);
            m_camera.setPosition(vec3{ 0,0,0});
            m_camera.lookAt(vec3{ 0,0,0.f});
            m_camera.setProjection(pyr::PerspectiveProjection{});
            m_camController.setCamera(&m_camera);
            m_ggxShader->addBinding({ .label = "CameraBuffer",   .bufferRef = pcameraBuffer });
            m_RDG.addPass(&m_SSAOPass);
            m_RDG.addPass(&m_forwardPass);
            m_RDG.addPass(&m_depthPrePass);
            m_RDG.getResourcesManager().addProduced(&m_depthPrePass, "depthBuffer");
            m_RDG.getResourcesManager().addProduced(&m_SSAOPass, "ssaoTexture_blurred");
            m_RDG.getResourcesManager().addProduced(&m_SSAOPass, "ssaoTexture");
            m_RDG.getResourcesManager().addRequirement(&m_SSAOPass, "depthBuffer");
            m_RDG.getResourcesManager().linkResource(&m_depthPrePass, "depthBuffer", &m_SSAOPass);
            m_RDG.getResourcesManager().linkResource(&m_SSAOPass, "ssaoTexture_blurred", &m_forwardPass);
            m_forwardPass.boundCamera = &m_camera;
            bool bIsGraphValid = m_RDG.getResourcesManager().checkResourcesValidity();
#pragma endregion RDG
            auto& device = pyr::Engine::device();

            std::array<pyr::FrameBuffer, 6> framebuffers;
            for (int i = 0; i < 6; i++)
            {

                framebuffers[i] = pyr::FrameBuffer{ device.getWinWidth(), device.getWinHeight(), pyr::FrameBuffer::COLOR_0 };
            }

            update(0.0F);

        }

        void update(float delta) override
        {
            m_camController.processUserInputs(delta);
            pcameraBuffer->setData(CameraBuffer::data_t{
                .mvp = m_camera.getViewProjectionMatrix(), 
                .pos = m_camera.getPosition()
            });
            pinvCameBuffer->setData(InverseCameraBuffer::data_t{
                .inverseViewProj = m_camera.getViewProjectionMatrix().Invert(),
                .inverseProj = m_camera.getProjectionMatrix().Invert(),
                .Proj = m_camera.getProjectionMatrix()
                            });
        }

        void render() override

        {
            pyr::Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            pyr::RenderProfiles::pushRasterProfile(pyr::RasterizerProfile::NOCULL_RASTERIZER);
            pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTWRITE_DEPTH);
 
            m_ggxShader->bindConstantBuffer("CameraBuffer", pcameraBuffer);
            m_depthPrePass.getDepthPassEffect()->bindConstantBuffer("CameraBuffer", pcameraBuffer);
            m_SSAOPass.getSSAOEffect()->bindConstantBuffer("InverseCameraBuffer", pinvCameBuffer);
            m_SSAOPass.getSSAOEffect()->bindConstantBuffer("CameraBuffer", pcameraBuffer);
            m_forwardPass.getSkyboxEffect()->bindConstantBuffer("CameraBuffer", pcameraBuffer);
            
            m_RDG.execute();

            pyr::RenderProfiles::popDepthProfile();
            pyr::RenderProfiles::popRasterProfile();
            //m_RDG.debugWindow();

        }

        ~MaterialScene() { m_RDG.clearGraph(); }
    };
}