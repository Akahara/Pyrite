#pragma once

#include "display/IndexBuffer.h"
#include "display/InputLayout.h"
#include "display/Vertex.h"
#include "display/VertexBuffer.h"
#include "engine/Engine.h"
#include "scene/Scene.h"

namespace pye
{
    
    class TriangleScene : public pyr::Scene
    {
    private:

        using triangle_vertex_t = pyr::GenericVertex<pyr::POSITION>;

        std::vector<triangle_vertex_t> m_vertices;
        pyr::InputLayout m_layout;
        pyr::Effect m_baseEffect;

        pyr::VertexBuffer m_vbo;
        pyr::IndexBuffer m_ibo;

    public:

        TriangleScene()
        {
            m_vertices.resize(3);
            m_vertices[0].position = pyr::BaseVertex::toPosition({ -0.5, -0.5, 0 });
            m_vertices[1].position = pyr::BaseVertex::toPosition({ +0.5, -0.5, 0 });
            m_vertices[2].position = pyr::BaseVertex::toPosition({ +0, 0.5, 0 });

            m_layout = pyr::InputLayout::MakeLayoutFromVertex<triangle_vertex_t>();

            m_baseEffect = pyr::ShaderManager::makeEffect(L"res/shaders/default.fx", m_layout);

            m_vbo = pyr::VertexBuffer(m_vertices);
            m_ibo = pyr::IndexBuffer({0,2,1});

        }

        void update(double delta) override
        {

        }

        void render() override

        {
            pyr::Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            m_baseEffect.bind();
            m_vbo.bind();
            m_ibo.bind();
            pyr::Engine::d3dcontext().DrawIndexed(static_cast<UINT>(m_ibo.getIndicesCount()), 0, 0);
            
        }
    };
}