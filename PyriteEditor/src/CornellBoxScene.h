#pragma once

#include "imgui.h"
#include "display/DebugDraw.h"
#include "display/IndexBuffer.h"
#include "display/InputLayout.h"
#include "display/Vertex.h"
#include "display/VertexBuffer.h"
#include "display/RenderGraph/RenderGraph.h"
#include "display/RenderGraph/BuiltinPasses/BuiltinPasses.h"
#include "display/RenderGraph/BuiltinPasses/RSMComputePass.h"
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
        pyr::BuiltinPasses::ReflectiveShadowMapComputePass  rsmPass;

        pyr::Camera m_camera;
        pyr::FreecamController m_camController;

        pyr::Texture brdfLUT;
        std::shared_ptr<pyr::Cubemap> specularCubemap;
        std::shared_ptr<pyr::Cubemap> m_irradianceMap;
        CubemapBuilderScene cubemapScene = CubemapBuilderScene();


    public:

        /// ------------------------------------------------------------------------------------------------------------------------------- ///
        
        CornellBoxScene()
        {

#pragma region RDG
            m_camera.setPosition(vec3{ 0,0,0 });
            m_camera.lookAt(vec3{ 0,0,0.f });
            m_camera.setProjection(pyr::PerspectiveProjection{});
            m_camController.setCamera(&m_camera);
            SceneRenderGraph.addPass(&m_depthPrePass);
            SceneRenderGraph.addPass(&m_SSAOPass);
            SceneRenderGraph.addPass(&m_forwardPass);
            SceneRenderGraph.addPass(&rsmPass);
            SceneRenderGraph.addPass(&m_billboardsPass);
            SceneRenderGraph.getResourcesManager().addProduced(&m_depthPrePass, "depthBuffer");
            SceneRenderGraph.getResourcesManager().addProduced(&m_SSAOPass, "ssaoTexture_blurred");
            SceneRenderGraph.getResourcesManager().addProduced(&m_SSAOPass, "ssaoTexture");
            SceneRenderGraph.getResourcesManager().addRequirement(&m_SSAOPass, "depthBuffer");

            SceneRenderGraph.getResourcesManager().linkResource(&m_depthPrePass, "depthBuffer", &m_SSAOPass);
            SceneRenderGraph.getResourcesManager().linkResource(&m_depthPrePass, "depthBuffer", &m_forwardPass);
            SceneRenderGraph.getResourcesManager().linkResource(&m_SSAOPass, "ssaoTexture_blurred", &m_forwardPass);
            bool bIsGraphValid = SceneRenderGraph.getResourcesManager().checkResourcesValidity();
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

            pye::Editor::Get().UpdateRegisteredActors(SceneActors);
            pye::EditorSceneInjector::InjectEditorToolsToScene(*this);
        }

        /// ------------------------------------------------------------------------------------------------------------------------------- ///
        
        void update(float delta) override
        {
            m_camController.processUserInputs(delta);
        }

        /// ------------------------------------------------------------------------------------------------------------------------------- ///
        
        void render() override
        {
            pyr::Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            pyr::RenderProfiles::pushRasterProfile(pyr::RasterizerProfile::NOCULL_RASTERIZER);
            pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTWRITE_DEPTH);

            SceneRenderGraph.execute(pyr::RenderContext{ .ActorsToRender = SceneActors, .contextCamera = &m_camera });

            pyr::RenderProfiles::popDepthProfile();
            pyr::RenderProfiles::popRasterProfile();

            if (ImGui::Button("Recompute IBL"))
            {
                auto renderDoc = pyr::RenderDoc::Get();
                if (renderDoc)    renderDoc->StartFrameCapture(nullptr, nullptr);
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
                if (renderDoc)    renderDoc->EndFrameCapture(nullptr, nullptr);
            }

            pye::EditorSceneInjector::OnRender();
        }

    };
}