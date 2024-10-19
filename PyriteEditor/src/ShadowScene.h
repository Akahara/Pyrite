#pragma once

#include "display/IndexBuffer.h"
#include "display/InputLayout.h"
#include "display/Vertex.h"
#include "display/VertexBuffer.h"
#include "engine/Engine.h"
#include "scene/Scene.h"

namespace pye
{
    class ShadowScene : public pyr::Scene
    {
    private:

        pyr::GraphicalResourceRegistry m_registry;

        std::vector<std::shared_ptr<pyr::Model>> m_catModels = pyr::MeshImporter::ImportMeshesFromFile(L"res/meshes/ConcreteStatue/concrete_cat_statue_4k.gltf");
        std::vector<std::shared_ptr<pyr::Model>> m_tableModels = pyr::MeshImporter::ImportMeshesFromFile(L"res/meshes/Table/round_wooden_table_01_4k.gltf");

        pyr::BuiltinPasses::ForwardPass     m_forwardPass;
        pyr::BuiltinPasses::SSAOPass        m_SSAOPass;
        pyr::BuiltinPasses::DepthPrePass    m_depthPrePass;

        pye::EditorPasses::WorldHUDPass     m_editorHUD;
        pye::EditorPasses::PickerPass       m_picker;
        pyr::RenderGraph m_RDG;

        pyr::Camera m_camera;
        pyr::FreecamController m_camController;

        pyr::Texture brdfLUT;
        std::shared_ptr<pyr::Cubemap> specularCubemap;
        std::shared_ptr<pyr::Cubemap> m_irradianceMap;
        CubemapBuilderScene cubemapScene = CubemapBuilderScene();

        std::shared_ptr<pyr::InverseCameraBuffer>   pinvCameBuffer = std::make_shared<pyr::InverseCameraBuffer>();
        std::shared_ptr<pyr::CameraBuffer>          pcameraBuffer = std::make_shared<pyr::CameraBuffer>();
        std::vector<pyr::StaticMesh> sceneMeshes;
        pye::WidgetsContainer HUD;
        pye::widgets::LightCollectionWidget LightCollectionWidget;
 
    public:

        ShadowScene()
        {
            HUD.widgets.push_back(&LightCollectionWidget);
            cubemapScene.ComputeIBLCubemaps();
            specularCubemap = cubemapScene.OutputCubemaps.SpecularFiltered;
            m_irradianceMap = cubemapScene.OutputCubemaps.Irradiance;
            m_registry.keepHandleToCubemap(*cubemapScene.OutputCubemaps.Cubemap);
            m_forwardPass.m_skybox = *cubemapScene.OutputCubemaps.Cubemap;
            brdfLUT = cubemapScene.BRDF_Lut;

            m_camera.setProjection(pyr::PerspectiveProjection{});

#pragma region RDG
            m_camController.setCamera(&m_camera);
            m_RDG.addPass(&m_depthPrePass);
            m_RDG.addPass(&m_SSAOPass);
            m_RDG.addPass(&m_forwardPass);
            m_RDG.addPass(&m_editorHUD);
            m_RDG.addPass(&m_picker);
            m_RDG.getResourcesManager().addProduced(&m_depthPrePass, "depthBuffer");
            m_RDG.getResourcesManager().addProduced(&m_SSAOPass, "ssaoTexture_blurred");
            m_RDG.getResourcesManager().addProduced(&m_SSAOPass, "ssaoTexture");
            m_RDG.getResourcesManager().addRequirement(&m_SSAOPass, "depthBuffer");

            m_RDG.getResourcesManager().linkResource(&m_depthPrePass, "depthBuffer", &m_SSAOPass);
            m_RDG.getResourcesManager().linkResource(&m_depthPrePass, "depthBuffer", &m_forwardPass);
            m_RDG.getResourcesManager().linkResource(&m_depthPrePass, "depthBuffer", &m_picker);
            m_RDG.getResourcesManager().linkResource(&m_SSAOPass, "ssaoTexture_blurred", &m_forwardPass);
            m_forwardPass.boundCamera = &m_camera;
            m_picker.boundCamera = &m_camera;
            m_editorHUD.boundCamera = &m_camera;
            bool bIsGraphValid = m_RDG.getResourcesManager().checkResourcesValidity();
#pragma endregion RDG
            for (const auto& model : m_catModels)
            {
                sceneMeshes.push_back(pyr::StaticMesh{ model });
                sceneMeshes.back().GetTransform().scale = { 10,10,10 };
            }

            for (const auto& model : m_tableModels)
            {
                sceneMeshes.push_back(pyr::StaticMesh{ model });
                sceneMeshes.back().GetTransform().scale = { 8,8,8};
                sceneMeshes.back().GetTransform().position = { 0,-8,0};
            }

            for (const auto& m : sceneMeshes)
            {
                SceneActors.meshes.push_back(&m);
            }
            SceneActors.lights.Points.push_back({});
            SceneActors.lights.Points.back().specularFactor = 10.F;
            SceneActors.lights.Points.back().GetTransform().position = { 3,5,0 };
            m_camera.setPosition({ -4, 3 ,8 });
            m_camera.lookAt({ 0,0,0 });
        }

        void update(float delta) override
        {
            static float elapsed = 0.0F;
            elapsed += 3*delta;
            m_camController.processUserInputs(delta);
            SceneActors.lights.Points.back().GetTransform().position = { 5 * sin(elapsed),5,  5 * cos(elapsed)};
        }

        void render() override
        {
            pcameraBuffer->setData(pyr::CameraBuffer::data_t{
               .mvp = m_camera.getViewProjectionMatrix(),
               .pos = m_camera.getPosition()
                });
            pinvCameBuffer->setData(pyr::InverseCameraBuffer::data_t{
                .inverseViewProj = m_camera.getViewProjectionMatrix().Invert(),
                .inverseProj = m_camera.getProjectionMatrix().Invert(),
                .Proj = m_camera.getProjectionMatrix()
                });


            pyr::Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            pyr::RenderProfiles::pushRasterProfile(pyr::RasterizerProfile::CULLBACK_RASTERIZER);
            pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTWRITE_DEPTH);

            m_depthPrePass.getDepthPassEffect()->bindConstantBuffer("CameraBuffer", pcameraBuffer);
            m_SSAOPass.getSSAOEffect()->bindConstantBuffer("InverseCameraBuffer", pinvCameBuffer);
            m_SSAOPass.getSSAOEffect()->bindConstantBuffer("CameraBuffer", pcameraBuffer);
            m_forwardPass.getSkyboxEffect()->bindConstantBuffer("CameraBuffer", pcameraBuffer);

            m_RDG.execute(pyr::RenderContext{ SceneActors });

            pyr::RenderProfiles::popDepthProfile();
            pyr::RenderProfiles::popRasterProfile();
        

            HUD.Render();
        }
    };
}