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

namespace pye
{

    class ForwardPassScene : public pyr::Scene
    {
    private:

        pyr::InputLayout m_layout;
        pyr::Effect* m_baseEffect;

        pyr::RenderGraph m_RDG;

        pyr::Mesh cubeMesh;
        pyr::Model cubeModel;
        pyr::StaticMesh cubeInstance;
        pyr::Material cubeMat;

        pyr::Camera m_camera;
        pyr::FreecamController m_camController;

        pyr::GraphicalResourceRegistry m_grr;
        pyr::Texture m_breadbug;

        pyr::BuiltinPasses::ForwardPass m_forwardPass;
        pyr::BuiltinPasses::SSAOPass m_SSAOPass;
        pyr::BuiltinPasses::DepthPrePass m_depthPrePass;

        using CameraBuffer = pyr::ConstantBuffer < InlineStruct(mat4 mvp; alignas(16) vec3 pos) > ;
        using InverseCameraBuffer = pyr::ConstantBuffer < InlineStruct(mat4 inverseViewProj;  mat4 inverseProj; alignas(16) mat4 Proj) > ;

        std::shared_ptr<CameraBuffer>           pcameraBuffer   = std::make_shared<CameraBuffer>();
        std::shared_ptr<InverseCameraBuffer>    pinvCameBuffer  = std::make_shared<InverseCameraBuffer>();

    public:

        ForwardPassScene()
        {
            drawDebugSetCamera(&m_camera);
            // Import shader and bind cbuffers
            m_layout = pyr::InputLayout::MakeLayoutFromVertex<pyr::Mesh::mesh_vertex_t>();

            m_baseEffect = m_grr.loadEffect(L"res/shaders/mesh.fx", m_layout); // todo say this is the default shader of materials

            m_baseEffect->addBinding({ .label = "CameraBuffer",   .bufferRef = pcameraBuffer });

            // Create axes and material (everything is kinda default here)
            const char PATH[] = "res/meshes/sponza.obj";

            std::vector<pyr::MaterialMetadata> mats  = pyr::MeshImporter::FetchMaterialPaths(PATH);

            cubeMesh        = pyr::MeshImporter::ImportMeshFromFile(PATH, 1);
            cubeModel       = pyr::Model{ cubeMesh };
            cubeInstance    = pyr::StaticMesh{ cubeModel };
            cubeInstance.setBaseMaterial(std::make_shared<pyr::Material>(m_baseEffect));

            cubeInstance.loadSubmeshesMaterial(mats);

            // Setup this scene's rendergraph
            m_RDG.addPass(&m_depthPrePass);
            m_RDG.addPass(&m_SSAOPass);
            m_RDG.addPass(&m_forwardPass);

            m_forwardPass.addMeshToPass(&cubeInstance);
            m_depthPrePass.addMeshToPass(&cubeInstance);

            // Setup the camera
            m_camera.setProjection(pyr::PerspectiveProjection{});
            m_camController.setCamera(&m_camera);

            m_RDG.linkResource(&m_depthPrePass, "depthBuffer", &m_SSAOPass);
            m_RDG.linkResource(&m_SSAOPass, "ssaoTexture", &m_forwardPass);

            m_RDG.ensureGraphValidity();
        }

        void update(float delta) override
        {
            static float elapsed = 0;
            elapsed += delta;

            m_camController.processUserInputs(delta);
            
            // Update Cbuffers
            pcameraBuffer->setData(CameraBuffer::data_t{ .mvp = m_camera.getViewProjectionMatrix(), .pos = m_camera.getPosition()});
            pinvCameBuffer->setData(InverseCameraBuffer::data_t{
                .inverseViewProj = m_camera.getViewProjectionMatrix().Invert(),
                .inverseProj = m_camera.getProjectionMatrix().Invert(),
                .Proj = m_camera.getProjectionMatrix()
                });
        }
        
        void render() override

        {
          
            ImGui::Begin("cam");
            static float ao_scale = 0.5f;
            if (vec3 euler = m_camera.getRotation().ToEuler(); ImGui::DragFloat3("rot", &euler.x, .1f))
              m_camera.setRotation(quat::CreateFromYawPitchRoll(euler));
            if (vec3 p = m_camera.getPosition(); ImGui::DragFloat3("pos", &p.x))
              m_camera.setPosition(p);
            if (ImGui::SliderFloat("Ambiant occlusion scale", &ao_scale, 0, 1))
                m_baseEffect->setUniform<float>("ao_scale", ao_scale);
            ImGui::End();

            pyr::RenderProfiles::pushRasterProfile(pyr::RasterizerProfile::NOCULL_RASTERIZER);
            pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTWRITE_DEPTH);
            pyr::Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            m_baseEffect->uploadAllBindings();

            m_forwardPass.getSkyboxEffect()->bindConstantBuffer("CameraBuffer", pcameraBuffer);
            m_depthPrePass.getDepthPassEffect()->bindConstantBuffer("CameraBuffer", pcameraBuffer);
            m_SSAOPass.getSSAOEffect()->bindConstantBuffer("InverseCameraBuffer", pinvCameBuffer);
            m_SSAOPass.getSSAOEffect()->bindConstantBuffer("CameraBuffer", pcameraBuffer);
            m_baseEffect->bindConstantBuffer("InverseCameraBuffer", pinvCameBuffer);

            m_RDG.execute();

            pyr::drawDebugLine(vec3::Zero, vec3::UnitX * 10.f, { 1,0,0,1 });
            pyr::drawDebugLine(vec3::Zero, vec3::UnitY * 10.f, { 0,1,0,1 });
            pyr::drawDebugLine(vec3::Zero, vec3::UnitZ * 10.f, { 0,0,1,1 });
            pyr::drawDebugBox(vec3::Zero, vec3(5.f));

            pyr::RenderProfiles::popDepthProfile();
            pyr::RenderProfiles::popRasterProfile();



        }

        ~ForwardPassScene() override
        {
            m_RDG.clearGraph();
        }
    };
}
