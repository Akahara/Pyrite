#pragma once

#include "RawMeshData.h"
#include "display/IndexBuffer.h"
#include "display/VertexBuffer.h"


namespace pyr
{

class Material;
class Model
{
public:
    using SubmeshesMaterialTable = std::vector<std::shared_ptr<Material>>;

private:

    std::shared_ptr<const RawMeshData> m_meshData;
    SubmeshesMaterialTable m_defaultMaterialTable;

    VertexBuffer m_vbo;
    IndexBuffer m_ibo;

public:


    Model() = default;
    Model(const std::shared_ptr<const RawMeshData>& rawMeshData) 
        : m_meshData(rawMeshData)
    {
        m_vbo = VertexBuffer(rawMeshData->getVertices());
        m_ibo = IndexBuffer(rawMeshData->getIndices());
    }

    Model(const std::shared_ptr<const RawMeshData>& rawMeshData, const SubmeshesMaterialTable& defaultMaterials)
        : Model(rawMeshData)
    {
        m_defaultMaterialTable = defaultMaterials;
    }
    void bind() const
    {
        m_vbo.bind();
        m_ibo.bind();
    }

    const SubmeshesMaterialTable& getDefaultSubmeshesMaterials() const noexcept { return m_defaultMaterialTable; }
    const std::shared_ptr<const RawMeshData>& getRawMeshData() const noexcept { return m_meshData; }
};

}
