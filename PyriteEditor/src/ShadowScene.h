#pragma once

#include "world/Material.h"
#include "imgui.h"
#include "display/IndexBuffer.h"
#include "display/InputLayout.h"
#include "display/Vertex.h"
#include "display/VertexBuffer.h"
#include "engine/Engine.h"
#include "scene/Scene.h"
#include "world/Tools/SceneRenderTools.h"
#include "display/DebugDraw.h"

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

        pyr::Camera orthoCam;
        pyr::Camera* currentCam;

 
    public:

        ShadowScene()
        {
            currentCam = &m_camera;
            HUD.widgets.push_back(&LightCollectionWidget);
            cubemapScene.ComputeIBLCubemaps();
            specularCubemap = cubemapScene.OutputCubemaps.SpecularFiltered;
            m_irradianceMap = cubemapScene.OutputCubemaps.Irradiance;
            m_registry.keepHandleToCubemap(*cubemapScene.OutputCubemaps.Cubemap);
            m_forwardPass.m_skybox = *cubemapScene.OutputCubemaps.Cubemap;
            brdfLUT = cubemapScene.BRDF_Lut;
            const pyr::Effect* ggxShader = pyr::MaterialBank::GetDefaultGGXShader();
            ggxShader->bindTexture(brdfLUT, "brdfLUT");
            ggxShader->bindCubemap(*m_irradianceMap, "irrandiance_map");
            ggxShader->bindCubemap(*specularCubemap, "prefilterMap");

#pragma region RDG

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
            m_forwardPass.boundCamera   = currentCam;
            m_picker.boundCamera = currentCam;
            m_editorHUD.boundCamera = currentCam;
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
            SceneActors.lights.Spots.push_back({});
            SceneActors.lights.Spots.back().GetTransform().position = { -2.3,3.39,-0.3 };
            SceneActors.lights.Spots.back().GetTransform().rotation = { 0.69, -0.724 };
            SceneActors.lights.Spots.back().strength = 300;
            SceneActors.lights.Spots.back().insideAngle = 0.650;
            SceneActors.lights.Spots.back().outsideAngle = 0;
            SceneActors.lights.Spots.back().isOn = false;

            SceneActors.lights.Points.push_back({});
            SceneActors.lights.Points.back().GetTransform().position = { -2.3,3.39,-0.3 };
            SceneActors.lights.Points.back().specularFactor = 20;
            SceneActors.lights.Points.back().isOn = true;
            SceneActors.lights.Points.back().shadowMode = pyr::DynamicShadow;
            
            m_camera.setProjection(pyr::PerspectiveProjection{});
            m_camera.setPosition({ -4, 3 ,8 });
            m_camera.lookAt({ 0,0,0 });

            orthoCam.setProjection(pyr::OrthographicProjection{.width = 30.F, .height = 30.F, .zNear = -100.f, .zFar = 100.F});
            orthoCam.setPosition({ 0,5,0 });

            drawDebugSetCamera(&m_camera);
        }

        void update(float delta) override
        {
            static float elapsed = 0.0F;
            elapsed += 3*delta;
            m_camController.setCamera(currentCam);
            m_camController.processUserInputs(delta);
            //SceneActors.lights.Points.back().GetTransform().position = { 5 * sin(elapsed),5,  5 * cos(elapsed)};

        }

        void render() override
        {
           // drawDebugCamera(orthoCam);
            pcameraBuffer->setData(pyr::CameraBuffer::data_t{
               .mvp = currentCam->getViewProjectionMatrix(),
               .pos = currentCam->getPosition()
                });
            pinvCameBuffer->setData(pyr::InverseCameraBuffer::data_t{
                .inverseViewProj    = currentCam->getViewProjectionMatrix().Invert(),
                .inverseProj = currentCam->getProjectionMatrix().Invert(),
                .Proj = currentCam->getProjectionMatrix()
                });

            //pyr::Texture shadowTexture = pyr::SceneRenderTools::MakeSceneDepth(SceneActors, orthoCam);
            //pyr::Cubemap omniShadowTexture = pyr::SceneRenderTools::MakeSceneDepthCubemapFromPoint(SceneActors, SceneActors.lights.Points.back().GetTransform().position, 512);

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