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
#include "world/Lights/Light.h"
#include "world/Mesh/MeshImporter.h"
#include "display/GraphicalResource.h"
#include "display/RenderProfiles.h"
#include "world/RayCasting.h"
#include <imfilebrowser.h>
#include <array>

#include "editor/views/widget.h"
#include "editor/views/EditorUI.h"
#include "editor/views/Lights/widget_lights.h"
#include <inputs/UserInputs.h>

namespace pye
{
    class CornellBoxScene : public pyr::Scene
    {
    private:

        pyr::GraphicalResourceRegistry m_registry;
        pyr::Effect* m_ggxShader;
        pyr::Effect* m_equiproj;


        std::vector<std::shared_ptr<pyr::Model>> m_cornellBoxModels = pyr::MeshImporter::ImportMeshesFromFile(L"res/meshes/CornellBox/scene.gltf");
        std::vector<pyr::StaticMesh> sceneMeshes;


        pyr::BuiltinPasses::ForwardPass     m_forwardPass;
        pyr::BuiltinPasses::SSAOPass        m_SSAOPass;
        pyr::BuiltinPasses::DepthPrePass    m_depthPrePass;
        pyr::BuiltinPasses::BillboardsPass  m_billboardsPass;
        pye::EditorPasses::PickerPass m_picker;
        pye::EditorPasses::WorldHUDPass m_editorHUD;
        pyr::RenderGraph m_RDG;

        pyr::Camera m_camera;
        pyr::FreecamController m_camController;

        std::shared_ptr<pyr::InverseCameraBuffer>   pinvCameBuffer  = std::make_shared<pyr::InverseCameraBuffer>();
        std::shared_ptr<pyr::CameraBuffer>          pcameraBuffer   = std::make_shared<pyr::CameraBuffer>();


        pyr::Texture brdfLUT;
        std::shared_ptr<pyr::Cubemap> specularCubemap;
        std::shared_ptr<pyr::Cubemap> m_irradianceMap;
        CubemapBuilderScene cubemapScene = CubemapBuilderScene();

        pye::widgets::WidgetsContainer HUD;
        pye::widgets::LightCollectionWidget LightCollectionWidget;

    public:

        /// ------------------------------------------------------------------------------------------------------------------------------- ///
        
        CornellBoxScene()
        {
            HUD.widgets.push_back(&LightCollectionWidget);

            cubemapScene.ComputeIBLCubemaps();
            specularCubemap = cubemapScene.OutputCubemaps.SpecularFiltered;
            m_irradianceMap = cubemapScene.OutputCubemaps.Irradiance;
            m_registry.keepHandleToCubemap(*cubemapScene.OutputCubemaps.Cubemap);
            m_forwardPass.m_skybox = *cubemapScene.OutputCubemaps.Cubemap;
            brdfLUT = cubemapScene.BRDF_Lut;

            m_ggxShader = m_registry.loadEffect(L"res/shaders/ggx.fx", pyr::InputLayout::MakeLayoutFromVertex<pyr::RawMeshData::mesh_vertex_t>());
            brdfLUT = m_registry.loadTexture(L"res/textures/pbr/brdfLUT.png");
            m_ggxShader->bindTexture(brdfLUT, "brdfLUT");
            m_ggxShader->bindCubemap(*m_irradianceMap, "irrandiance_map");
            m_ggxShader->bindCubemap(*specularCubemap, "prefilterMap");
#pragma region RDG
            m_camera.setPosition(vec3{ 0,0,0 });
            m_camera.lookAt(vec3{ 0,0,0.f });
            m_camera.setProjection(pyr::PerspectiveProjection{});
            m_camController.setCamera(&m_camera);
            m_ggxShader->addBinding({ .label = "CameraBuffer",   .bufferRef = pcameraBuffer });
            m_RDG.addPass(&m_depthPrePass);
            m_RDG.addPass(&m_SSAOPass);
            m_RDG.addPass(&m_forwardPass);
            m_RDG.addPass(&m_billboardsPass);
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
            bool bIsGraphValid = m_RDG.getResourcesManager().checkResourcesValidity();
#pragma endregion RDG

            sceneMeshes.reserve(m_cornellBoxModels.size());
            for (const std::shared_ptr<pyr::Model>& model : m_cornellBoxModels)
            {
                sceneMeshes.emplace_back(pyr::StaticMesh{ model });
                sceneMeshes.back().GetTransform().scale = { 10,10,10 };
            }

            SceneActors.lights.Points.push_back({});
            SceneActors.lights.Spots.push_back({});
            SceneActors.lights.Points.back().GetTransform().position = { 0,-5.F,0 };
            SceneActors.lights.Spots.back().GetTransform().position = { 0,14.F,28 };
            SceneActors.lights.Spots.back().GetTransform().rotation = { 0,0, -1.0f };
            SceneActors.lights.Spots.back().strength = 26.f;
            SceneActors.lights.Spots.back().shadowMode = pyr::DynamicShadow;

            for (const auto& m : sceneMeshes)
            {
                SceneActors.meshes.push_back(&m); // < this assumes that the scene manager will eventually clear them at the end of the frame, which is a stupid idea. I hate me
            }
        }

        /// ------------------------------------------------------------------------------------------------------------------------------- ///
        
        void update(float delta) override
        {
            m_camController.processUserInputs(delta);
        }

        /// ------------------------------------------------------------------------------------------------------------------------------- ///
        
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
            pyr::RenderProfiles::pushRasterProfile(pyr::RasterizerProfile::NOCULL_RASTERIZER);
            pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTWRITE_DEPTH);

            m_depthPrePass.getDepthPassEffect()->bindConstantBuffer("CameraBuffer", pcameraBuffer);
            m_SSAOPass.getSSAOEffect()->bindConstantBuffer("InverseCameraBuffer", pinvCameBuffer);
            m_SSAOPass.getSSAOEffect()->bindConstantBuffer("CameraBuffer", pcameraBuffer);

            m_RDG.execute(pyr::RenderContext{ SceneActors });

            pyr::RenderProfiles::popDepthProfile();
            pyr::RenderProfiles::popRasterProfile();

            HUD.Render();

        }

    };
}