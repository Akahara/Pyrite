#pragma once
#include "Model.h"
#include "world/Material.h"
#include "world/Transform.h"

namespace pyr
{
    
class StaticMesh
{
private:

    const Model* m_model;
    Transform m_transform;
    std::shared_ptr<Material> m_material;

    std::unordered_map<int, std::shared_ptr<Material>> m_submeshesMaterial;

public:

    StaticMesh() = default;
    StaticMesh(const Model& model) 
        : m_model(&model)
    {}

    void setBaseMaterial(std::shared_ptr<Material> mat)
    {
        m_material = mat;
    }

    std::shared_ptr<Material> getMaterialOfIndex(int matId) const
    {
        if (matId < 0 || !m_submeshesMaterial.contains(matId)) return nullptr;
        return m_submeshesMaterial.at(matId); // todo figure out where the indice offset comes from (should be assimp loading starting at index 1)
    }

    void loadSubmeshesMaterial(const std::vector<MaterialMetadata>& inOrderMetadata)
    {
        int i = 0;
        for (const auto& metadata : inOrderMetadata)
        {
            auto newMat = std::make_shared<Material>();
            newMat->loadMaterialFromMetadata(metadata);
            m_submeshesMaterial[i++] = newMat;
        }
    }

    const pyr::Model* getModel()       const { return m_model; }
    std::shared_ptr<pyr::Material> getMaterial() const { return m_material; }

    Transform& getTransform()       { return m_transform; }
    Transform getTransform() const  { return m_transform; }
    
    void bindModel()    const   { m_model->bind();   }
    void bindMaterial() const   { m_material->bind(); }
};

}