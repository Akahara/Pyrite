#pragma once

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
        using CameraBuffer = pyr::ConstantBuffer < InlineStruct(mat4 mvp; vec4 pos) > ;
        std::shared_ptr<CameraBuffer> pter;

    public:

        ForwardPassScene()
        {

            m_layout = pyr::InputLayout::MakeLayoutFromVertex<pyr::Mesh::mesh_vertex_t>();

            m_baseEffect = pyr::ShaderManager::makeEffect(L"res/shaders/mesh.fx", m_layout);
            cubeMat = pyr::Material(&m_baseEffect);

            cubeMesh = pyr::MeshImporter::ImportMeshFromFile("res/meshes/cube.obj");
            cubeModel = pyr::Model{ cubeMesh };
            cubeInstance = pyr::StaticMesh{ cubeModel, cubeMat };

            pyr::BuiltinPasses::s_ForwardPass.addMeshToPass(&cubeInstance);

            m_RDG.addPass(&pyr::BuiltinPasses::s_ForwardPass);

            m_camera.setPosition({ 1,2,5 });
            m_camera.setProjection(pyr::PerspectiveProjection{});
            m_camera.lookAt({0,0,0});
            m_camera.updateViewMatrix();

            pter = std::make_shared<CameraBuffer>();
            pter->setData(CameraBuffer::data_t{ .mvp = m_camera.getViewProjectionMatrix(), .pos = vec4{1,2,5,0} });
   

        }

        void update(double delta) override
        {
            auto mvp = m_camera.getViewProjectionMatrix();
            static float elapsed = 0;
            elapsed += delta;
            m_baseEffect.setUniform<float>("u_blue", (sin(elapsed) + 1) / 2.f);


            m_camera.setPosition({ 10 * sin(elapsed),5 , 10 * cos(elapsed)});
            m_camera.lookAt({ 0,0,0 });
            m_camera.updateViewMatrix();

            pter->setData(CameraBuffer::data_t{ .mvp = m_camera.getViewProjectionMatrix(), .pos = vec4{1,2,5,0} });
        }

        void render() override

        {
            pyr::Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            m_baseEffect.bindConstantBuffer("CameraBuffer", pter);
            m_RDG.execute();

        }

        ~ForwardPassScene()
        {
            m_RDG.clearGraph();
        }

    };
}
