#pragma once

#include <span>
#include <vector>
#include <string>

#include "display/IndexBuffer.h"
#include "display/Vertex.h"

namespace pyr
{

/////////////////////////////////////////////////////////////////

    // An index that represents at what point/index of the whole ibo we start a new submesh.
    struct SubMesh
    {
        IndexBuffer::size_type startIndex   = 0;
        IndexBuffer::size_type endIndex     = 0;
        size_t materialIndex                = 0;
#ifdef _DEBUG
        std::string matName;
#endif

        IndexBuffer::size_type getIndexCount() const noexcept { return endIndex - startIndex; }
    };

/////////////////////////////////////////////////////////////////
   
// Contains the actual 3D geometry, vertices and indices and submeshes.
class RawMeshData {

public:
    using mesh_vertex_t = GenericVertex<POSITION, NORMAL, UV>;
    using mesh_indice_t = IndexBuffer::size_type;

private:

    std::vector<SubMesh> m_submeshes;

    std::vector<mesh_vertex_t> m_vertices;
    std::vector<mesh_indice_t> m_indices;

public:

    RawMeshData() = default;
    RawMeshData(const std::vector<mesh_vertex_t>& vertices,
        const std::vector<mesh_indice_t>& indices)
        : m_vertices(vertices)
        , m_indices(indices)
    {}
    RawMeshData(const std::vector<mesh_vertex_t>& vertices,
        const std::vector<mesh_indice_t>& indices,
        const std::vector<SubMesh>& submeshes
        )
        : m_vertices(vertices)
        , m_indices(indices)
        , m_submeshes(submeshes)
    {}

    const std::vector<SubMesh>& getSubmeshes()      const noexcept { return  m_submeshes; };
    const std::vector<mesh_vertex_t>& getVertices() const noexcept { return m_vertices; }
    const std::vector<mesh_indice_t>& getIndices()  const noexcept { return m_indices; }


};

}
