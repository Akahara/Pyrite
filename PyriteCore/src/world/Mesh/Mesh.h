﻿#pragma once

#include <span>
#include <vector>

#include "display/IndexBuffer.h"
#include "display/Vertex.h"

namespace pyr
{

    /////////////////////////////////////////////////////////////////

    // An index that represents at what point/index of the whole ibo we start a new submesh.
    struct SubMesh
    {
        IndexBuffer::size_type startIndex;
    };

    /////////////////////////////////////////////////////////////////
    
    class Mesh {

    public:
        using mesh_vertex_t = GenericVertex<POSITION, UV>;
        using mesh_indice_t = IndexBuffer::size_type;

private:

    std::vector<SubMesh> m_submeshes;

    std::vector<mesh_vertex_t> m_vertices;
    std::vector<mesh_indice_t> m_indices;

public:

    Mesh() = default;
    Mesh(const std::vector<mesh_vertex_t>& vertices,
        const std::vector<mesh_indice_t>& indices)
        : m_vertices(vertices)
        , m_indices(indices)
    {}
    Mesh(const std::vector<mesh_vertex_t>& vertices,
        const std::vector<mesh_indice_t>& indices,
        const std::vector<SubMesh>& submeshes
        )
        : m_vertices(vertices)
        , m_indices(indices)
        , m_submeshes(submeshes)
    {}

    void addSubmesh(SubMesh submeshIndex) {}
    void addSubmesh(IndexBuffer::size_type submeshIndex) {}
    std::span<const SubMesh> getSubmeshes() const noexcept { return  std::span(m_submeshes); };

    const std::vector<mesh_vertex_t>& getVertices() const noexcept { return m_vertices; }
    const std::vector<mesh_indice_t>& getIndices() const noexcept { return m_indices; }


};

}
