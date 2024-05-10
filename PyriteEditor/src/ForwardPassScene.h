#pragma once

#include "imgui.h"
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

namespace pye
{

    class ForwardPassScene : public pyr::Scene
    {
    private:

        pyr::InputLayout m_layout;
        pyr::Effect m_baseEffect;

        pyr::RenderGraph m_RDG;
        pyr::Mesh cubeMesh;
        pyr::Model cubeModel;
        pyr::StaticMesh cubeInstance;
        pyr::Material cubeMat;

        pyr::Camera m_camera;
        pyr::FreecamController m_camController;
        using CameraBuffer = pyr::ConstantBuffer < InlineStruct(mat4 mvp; alignas(16) vec3 pos) > ;
        std::shared_ptr<CameraBuffer> pcameraBuffer;

        pyr::GraphicalResourceRegistry m_grr;
        pyr::Texture m_breadbug;
        pyr::BuiltinPasses::ForwardPass m_forwardPass;

    public:

        ForwardPassScene()
        {
            m_layout = pyr::InputLayout::MakeLayoutFromVertex<pyr::Mesh::mesh_vertex_t>();

            m_baseEffect = pyr::ShaderManager::makeEffect(L"res/shaders/mesh.fx", m_layout);
            cubeMat = pyr::Material(&m_baseEffect);

            cubeMesh = pyr::MeshImporter::ImportMeshFromFile("res/meshes/axes.obj");
            cubeModel = pyr::Model{ cubeMesh };
            cubeInstance = pyr::StaticMesh{ cubeModel, cubeMat };

            m_forwardPass.addMeshToPass(&cubeInstance);

            m_RDG.addPass(&m_forwardPass);

            m_camera.setPosition({ 1,2,5 });
            m_camera.setProjection(pyr::PerspectiveProjection{});
            m_camera.lookAt({0,0,0});
            m_camController.setCamera(&m_camera);

            pcameraBuffer = std::make_shared<CameraBuffer>();
            pcameraBuffer->setData(CameraBuffer::data_t{ .mvp = m_camera.getViewProjectionMatrix(), .pos = m_camera.getPosition() });
            m_breadbug = m_grr.loadTexture(L"res/textures/breadbug.dds");
        }

        void update(double delta) override
        {
            static float elapsed = 0;
            elapsed += delta;
            m_baseEffect.setUniform<float>("u_blue", (sin(elapsed) + 1) / 2.f);
            m_baseEffect.bindTexture(m_breadbug, "tex_breadbug");

            m_camController.processUserInputs(delta);

            pcameraBuffer->setData(CameraBuffer::data_t{ .mvp = m_camera.getViewProjectionMatrix(), .pos = m_camera.getPosition()});
        }

        void render() override

        {
          
            ImGui::Begin("cam");
            if (vec3 euler = m_camera.getRotation().ToEuler(); ImGui::DragFloat3("rot", &euler.x, .1f))
              m_camera.setRotation(quat::CreateFromYawPitchRoll(euler));
            if (vec3 p = m_camera.getPosition(); ImGui::DragFloat3("pos", &p.x))
              m_camera.setPosition(p);
            ImGui::End();

            pyr::RenderProfiles::pushRasterProfile(pyr::RasterizerProfile::NOCULL_RASTERIZER);
            pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTWRITE_DEPTH);
            pyr::Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            m_baseEffect.bindConstantBuffer("CameraBuffer", pcameraBuffer);
            m_forwardPass.getSkyboxEffect()->bindConstantBuffer("CameraBuffer", pcameraBuffer);
            m_RDG.execute();
            pyr::RenderProfiles::popDepthProfile();
            pyr::RenderProfiles::popRasterProfile();

        }

        ~ForwardPassScene()
        {
            m_RDG.clearGraph();
        }

    };
}
