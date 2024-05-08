#pragma once

#include "display/IndexBuffer.h"
#include "display/InputLayout.h"
#include "display/Vertex.h"
#include "display/VertexBuffer.h"
#include "display/RenderGraph/RenderGraph.h"
#include "display/RenderGraph/BuiltinPasses/BuiltinPasses.h"
#include "engine/Engine.h"
#include "scene/Scene.h"
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

        }

        void update(double delta) override
        {
        }

        void render() override

        {
            pyr::Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            m_RDG.execute();

        }
    };
}
