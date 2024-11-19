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

#include "editor/views/Materials/widget_material.h"
#include "editor/views/Rendergraph/widget_rendergraph.h"
#include "editor/views/EditorUI.h"
#include "editor/EditorSceneInjector.h"

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

        pyr::Camera m_camera;
        pyr::FreecamController m_camController;

        pyr::Texture brdfLUT;
        std::shared_ptr<pyr::Cubemap> specularCubemap;
        std::shared_ptr<pyr::Cubemap> m_irradianceMap;
        CubemapBuilderScene cubemapScene = CubemapBuilderScene();

        std::vector<pyr::StaticMesh> sceneMeshes;
 
    public:

        ShadowScene()
        {

#pragma region RDG
            SceneRenderGraph.addPass(&m_depthPrePass);
            SceneRenderGraph.addPass(&m_SSAOPass);
            SceneRenderGraph.addPass(&m_forwardPass);
            SceneRenderGraph.getResourcesManager().addProduced(&m_depthPrePass, "depthBuffer");
            SceneRenderGraph.getResourcesManager().addProduced(&m_SSAOPass, "ssaoTexture_blurred");
            SceneRenderGraph.getResourcesManager().addProduced(&m_SSAOPass, "ssaoTexture");
            SceneRenderGraph.getResourcesManager().addRequirement(&m_SSAOPass, "depthBuffer");

            SceneRenderGraph.getResourcesManager().linkResource(&m_depthPrePass, "depthBuffer", &m_SSAOPass);
            SceneRenderGraph.getResourcesManager().linkResource(&m_depthPrePass, "depthBuffer", &m_forwardPass);
            SceneRenderGraph.getResourcesManager().linkResource(&m_SSAOPass, "ssaoTexture_blurred", &m_forwardPass);
            bool bIsGraphValid = SceneRenderGraph.getResourcesManager().checkResourcesValidity();
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
            SceneActors.lights.Spots.back().GetTransform().position = { -2.3f,3.39f,-0.3f };
            SceneActors.lights.Spots.back().GetTransform().rotation = { 0.69f, -0.724f };
            SceneActors.lights.Spots.back().strength = 300.f;
            SceneActors.lights.Spots.back().insideAngle = 0.650f;
            SceneActors.lights.Spots.back().outsideAngle = 0.f;
            SceneActors.lights.Spots.back().isOn = false;

            SceneActors.lights.Points.push_back({});
            SceneActors.lights.Points.back().GetTransform().position = { -2.3f,3.39f,-0.3f };
            SceneActors.lights.Points.back().specularFactor = 20;
            SceneActors.lights.Points.back().isOn = true;
            SceneActors.lights.Points.back().shadowMode = pyr::DynamicShadow;
            
            m_camera.setProjection(pyr::PerspectiveProjection{});
            m_camera.setPosition({ -4, 3 ,8 });
            m_camera.lookAt({ 0,0,0 });

            pye::Editor::Get().UpdateRegisteredActors(SceneActors);
            pye::EditorSceneInjector::InjectEditorToolsToScene(*this);
        }

        void update(float delta) override
        {
            static float elapsed = 0.0F;
            elapsed += 3*delta;
            m_camController.setCamera(&m_camera);
            m_camController.processUserInputs(delta);
        }

        void render() override
        {

            pyr::Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            pyr::RenderProfiles::pushRasterProfile(pyr::RasterizerProfile::CULLBACK_RASTERIZER);
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