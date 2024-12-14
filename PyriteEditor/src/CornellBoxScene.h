#pragma once

#include "imgui.h"
#include "display/DebugDraw.h"
#include "display/IndexBuffer.h"
#include "display/InputLayout.h"
#include "display/Vertex.h"
#include "display/VertexBuffer.h"
#include "display/RenderGraph/RenderGraph.h"
#include "display/RenderGraph/BuiltinPasses/BuiltinPasses.h"
#include "display/RenderGraph/BuiltinPasses/RSMGIPass.h"
#include "display/RenderGraph/BuiltinPasses/RSMComputePass.h"
#include "display/RenderGraph/BuiltinPasses/Deferred_LightPass.h"
#include "display/RenderGraph/BuiltinPasses/Deferred_GeometryPass.h"
#include "display/RenderGraph/BuiltinPasses/SkyboxPass.h"
#include "display/RenderGraph/BuiltinPasses/ShadowComputePass.h"
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

        pyr::BuiltinPasses::Deferred_GeometryPass m_gPass;
        pyr::BuiltinPasses::Deferred_LightPass m_lightPass;

        pyr::BuiltinPasses::ForwardPass     m_forwardPass;
        pyr::BuiltinPasses::SSAOPass        m_SSAOPass;
        pyr::BuiltinPasses::DepthPrePass    m_depthPrePass;
        pyr::BuiltinPasses::BillboardsPass  m_billboardsPass;
        pyr::BuiltinPasses::ReflectiveShadowMapComputePass  RSMComputePass;
        pyr::BuiltinPasses::ReflectiveShadowMap_GlobalIlluminationPass  RSMGIPass;
        pyr::BuiltinPasses::ShadowComputePass  m_shadowComputePass;
        pyr::BuiltinPasses::SkyboxPass          m_skyboxPass;

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
            SceneRenderGraph.addPass(&m_gPass);

            SceneRenderGraph.addPass(&m_shadowComputePass);
            SceneRenderGraph.addPass(&m_SSAOPass);
            SceneRenderGraph.addPass(&RSMComputePass);
            SceneRenderGraph.addPass(&RSMGIPass);
            SceneRenderGraph.addPass(&m_forwardPass);
            SceneRenderGraph.addPass(&m_lightPass);
            
            SceneRenderGraph.addPass(&m_skyboxPass);
            
            SceneRenderGraph.addPass(&m_billboardsPass);

            SceneRenderGraph.getResourcesManager().addProduced(&m_depthPrePass, "depthBuffer");
            SceneRenderGraph.getResourcesManager().addProduced(&m_SSAOPass, "ssaoTexture_blurred");
            SceneRenderGraph.getResourcesManager().addProduced(&m_SSAOPass, "ssaoTexture");
            SceneRenderGraph.getResourcesManager().addProduced(&RSMComputePass, "RSM_WorldPos_TextureArray");
            SceneRenderGraph.getResourcesManager().addProduced(&RSMComputePass, "RSM_Normals_TextureArray");
            SceneRenderGraph.getResourcesManager().addProduced(&RSMComputePass, "RSM_Flux_TextureArray");
            SceneRenderGraph.getResourcesManager().addProduced(&RSMComputePass, "RSM_DepthBuffers_TextureArray");
            SceneRenderGraph.getResourcesManager().addProduced(&RSMComputePass, "RSM_LowRes_WorldPos_TextureArray");
            SceneRenderGraph.getResourcesManager().addProduced(&RSMComputePass, "RSM_LowRes_Normals_TextureArray");
            SceneRenderGraph.getResourcesManager().addProduced(&RSMComputePass, "RSM_LowRes_Flux_TextureArray");
            SceneRenderGraph.getResourcesManager().addProduced(&RSMComputePass, "RSM_LowRes_DepthBuffers_TextureArray");
            SceneRenderGraph.getResourcesManager().addProduced(&RSMGIPass, "GI_CompositeIndirectIllumination");
            SceneRenderGraph.getResourcesManager().addProduced(&m_gPass, "G_Buffer");
            SceneRenderGraph.getResourcesManager().addProduced(&m_lightPass, "Deferred_Lightpass");

            SceneRenderGraph.getResourcesManager().addProduced(&m_shadowComputePass, "Lightmaps_2D");
            SceneRenderGraph.getResourcesManager().addProduced(&m_shadowComputePass, "Lightmaps_3D");

            SceneRenderGraph.getResourcesManager().addRequirement(&m_SSAOPass, "depthBuffer");
            SceneRenderGraph.getResourcesManager().addRequirement(&m_skyboxPass, "depthBuffer");
            
            

            SceneRenderGraph.getResourcesManager().linkResource(&m_depthPrePass, "depthBuffer", &m_SSAOPass);
            SceneRenderGraph.getResourcesManager().linkResource(&m_depthPrePass, "depthBuffer", &m_forwardPass);
            SceneRenderGraph.getResourcesManager().linkResource(&m_depthPrePass, "depthBuffer", &m_skyboxPass);
            SceneRenderGraph.getResourcesManager().linkResource(&m_SSAOPass, "ssaoTexture_blurred", &m_forwardPass);
            SceneRenderGraph.getResourcesManager().linkResource(&RSMGIPass, "GI_CompositeIndirectIllumination", &m_forwardPass);
            SceneRenderGraph.getResourcesManager().linkResource(&m_shadowComputePass, "Lightmaps_3D", &m_forwardPass);
            SceneRenderGraph.getResourcesManager().linkResource(&m_shadowComputePass, "Lightmaps_2D", &m_forwardPass);

            SceneRenderGraph.getResourcesManager().linkResource(&RSMComputePass, "RSM_WorldPos_TextureArray", &RSMGIPass);
            SceneRenderGraph.getResourcesManager().linkResource(&RSMComputePass, "RSM_Normals_TextureArray", &RSMGIPass);
            SceneRenderGraph.getResourcesManager().linkResource(&RSMComputePass, "RSM_Flux_TextureArray", &RSMGIPass);
            SceneRenderGraph.getResourcesManager().linkResource(&RSMComputePass, "RSM_DepthBuffers_TextureArray", &RSMGIPass);

            SceneRenderGraph.getResourcesManager().linkResource(&RSMComputePass, "RSM_LowRes_WorldPos_TextureArray", &RSMGIPass);
            SceneRenderGraph.getResourcesManager().linkResource(&RSMComputePass, "RSM_LowRes_Normals_TextureArray", &RSMGIPass);
            SceneRenderGraph.getResourcesManager().linkResource(&RSMComputePass, "RSM_LowRes_Flux_TextureArray", &RSMGIPass);
            SceneRenderGraph.getResourcesManager().linkResource(&RSMComputePass, "RSM_LowRes_DepthBuffers_TextureArray", &RSMGIPass);


            SceneRenderGraph.getResourcesManager().linkResource(&m_gPass, "G_Buffer", &m_lightPass);

            bool bIsGraphValid = SceneRenderGraph.getResourcesManager().checkResourcesValidity();
#pragma endregion RDG

            sceneMeshes.reserve(m_cornellBoxModels.size());
            for (const std::shared_ptr<pyr::Model>& model : m_cornellBoxModels)
            {
                sceneMeshes.emplace_back(pyr::StaticMesh{ model });
                sceneMeshes.back().GetTransform().scale = { 10,10,10 };
            }

            SceneActors.lights.Spots.push_back({});
            SceneActors.lights.Spots.back().GetTransform().position = { 0,14.F,28 };
            SceneActors.lights.Spots.back().GetTransform().rotation = { 0,0, -1.0f };
            SceneActors.lights.Spots.back().strength = 26.f;
            SceneActors.lights.Spots.back().shadowMode = pyr::DynamicShadow_RSM;

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
                //m_forwardPass.m_skybox = *cubemapScene.OutputCubemaps.Cubemap;
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