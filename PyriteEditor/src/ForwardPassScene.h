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

#include "world/Tools/CommonConstantBuffers.h"

namespace pye
{

    class ForwardPassScene : public pyr::Scene
    {
    private:

        pyr::Camera m_camera;
        pyr::FreecamController m_camController;
        
        pyr::GraphicalResourceRegistry m_grr;

        pyr::RenderGraph m_RDG;
        pyr::BuiltinPasses::ForwardPass m_forwardPass;
        pyr::BuiltinPasses::SSAOPass m_SSAOPass;
        pyr::BuiltinPasses::DepthPrePass m_depthPrePass;

        std::shared_ptr<pyr::CameraBuffer>           pcameraBuffer   = std::make_shared<pyr::CameraBuffer>();
        std::shared_ptr<pyr::InverseCameraBuffer>    pinvCameBuffer  = std::make_shared<pyr::InverseCameraBuffer>();

        std::vector<pyr::StaticMesh> sceneMeshes;
        pyr::Cubemap m_irradianceMap;

    public:

        ForwardPassScene()
        {
            drawDebugSetCamera(&m_camera);
            // Import shader and bind cbuffers
            
            const char PATH[] = "res/meshes/chess_set_4k.gltf";
            static std::vector<std::shared_ptr<pyr::Model>> modelsOfFile = pyr::MeshImporter::ImportMeshesFromFile(PATH);
            m_irradianceMap = m_grr.loadCubemap(L"res/textures/pbr/irradiance_cubemap.dds");
            pyr::MaterialBank::GetDefaultGGXShader()->bindCubemap(m_irradianceMap, "irrandiance_map");

            // Setup this scene's rendergraph
            m_RDG.addPass(&m_depthPrePass);
            m_RDG.addPass(&m_SSAOPass);
            m_RDG.addPass(&m_forwardPass);

            sceneMeshes.reserve(modelsOfFile.size());

            int i = 0;
            for (const std::shared_ptr<pyr::Model>& model : modelsOfFile)
            {
                sceneMeshes.emplace_back(pyr::StaticMesh{ model });
                sceneMeshes.back().GetTransform().scale = { 30,30, 30 };
            }

            m_RDG.getResourcesManager().addProduced(&m_depthPrePass, "depthBuffer");
            m_RDG.getResourcesManager().addProduced(&m_SSAOPass, "ssaoTexture_blurred");
            m_RDG.getResourcesManager().addProduced(&m_SSAOPass, "ssaoTexture");

            m_RDG.getResourcesManager().addRequirement(&m_SSAOPass, "depthBuffer");

            m_RDG.getResourcesManager().linkResource(&m_depthPrePass, "depthBuffer", &m_SSAOPass);
            m_RDG.getResourcesManager().linkResource(&m_SSAOPass, "ssaoTexture_blurred", &m_forwardPass);

            bool bIsGraphValid = m_RDG.getResourcesManager().checkResourcesValidity();

            // Setup the camera
            m_camera.setProjection(pyr::PerspectiveProjection{});
            m_camController.setCamera(&m_camera);
            m_forwardPass.boundCamera = &m_camera;

            for (const auto& mesh : sceneMeshes)
            {
                SceneActors.meshes.push_back(&mesh);
            }
        }

        void update(float delta) override
        {
            static float elapsed = 0;
            elapsed += delta;

            m_camController.processUserInputs(delta);
            
            // Update Cbuffers
            pcameraBuffer->setData(pyr::CameraBuffer::data_t{ .mvp = m_camera.getViewProjectionMatrix(), .pos = m_camera.getPosition()});
            pinvCameBuffer->setData(pyr::InverseCameraBuffer::data_t{
                .inverseViewProj = m_camera.getViewProjectionMatrix().Invert(),
                .inverseProj = m_camera.getProjectionMatrix().Invert(),
                .Proj = m_camera.getProjectionMatrix()
                });
        }
        
        void render() override
        {
            ImGui::Begin("Forward Pass Scene");
            static vec3 sunPos = vec3{ 0,100,0 };
            if (ImGui::SliderFloat3("sunPos", &sunPos.x, -300, 300))
                pyr::MaterialBank::GetDefaultGGXShader()->setUniform<vec3>("sunPos", sunPos);

            ImGui::End();


            pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTWRITE_DEPTH);
            pyr::RenderProfiles::pushRasterProfile(pyr::RasterizerProfile::CULLBACK_RASTERIZER);
            pyr::Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            m_depthPrePass.getDepthPassEffect()->bindConstantBuffer("CameraBuffer", pcameraBuffer);
            m_SSAOPass.getSSAOEffect()->bindConstantBuffer("InverseCameraBuffer", pinvCameBuffer);
            m_SSAOPass.getSSAOEffect()->bindConstantBuffer("CameraBuffer", pcameraBuffer);

            m_RDG.execute(pyr::RenderContext{ SceneActors });

            pyr::drawDebugLine(vec3::Zero, vec3::UnitX * 10.f, { 1,0,0,1 });
            pyr::drawDebugLine(vec3::Zero, vec3::UnitY * 10.f, { 0,1,0,1 });
            pyr::drawDebugLine(vec3::Zero, vec3::UnitZ * 10.f, { 0,0,1,1 });
            pyr::drawDebugBox(vec3::Zero, vec3(5.f));

            pyr::RenderProfiles::popDepthProfile();
            pyr::RenderProfiles::popRasterProfile();

            m_RDG.debugWindow();

        }
    };
}
