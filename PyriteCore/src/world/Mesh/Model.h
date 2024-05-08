#pragma once
#include "Mesh.h"
#include "display/IndexBuffer.h"
#include "display/VertexBuffer.h"

namespace pyr
{

class Model
{
private:
    // vbo ibo

    const pyr::Mesh* m_meshData;
    VertexBuffer m_vbo;
    IndexBuffer m_ibo;

public:


    Model() = default;
    Model(const Mesh& rawMeshData) : m_meshData(&rawMeshData)
    {
        m_vbo = pyr::VertexBuffer(rawMeshData.getVertices());
        m_ibo = pyr::IndexBuffer(rawMeshData.getIndices());
    }

    void bind() const
    {
        m_vbo.bind();
        m_ibo.bind();
    }

    const pyr::Mesh* getRawMeshData() const { return m_meshData; }
    
};

}
