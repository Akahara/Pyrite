﻿#pragma once

#include <filesystem>

#include "Model.h"

#include "world/Material.h"
#include "world/Transform.h"
#include "world/Actor.h"

namespace pyr
{

    class StaticMesh : public Actor
    {
    private:

        std::shared_ptr<Model> m_model;
        Transform m_transform;

        Model::SubmeshesMaterialTable m_submeshesMaterials;

    public:


        StaticMesh() = default;
        StaticMesh(const std::shared_ptr<Model>& model)
            : m_model(model)
        {
            m_submeshesMaterials = model->getDefaultSubmeshesMaterials();
        }

        void overrideSubmeshMaterial(size_t materialLocalIndex, const std::shared_ptr<Material>& materialOverride)
        {
            if (materialLocalIndex >= m_submeshesMaterials.size())
            {
                return;
            }

            m_submeshesMaterials[materialLocalIndex] = materialOverride;
        }

        std::shared_ptr<const Material> getMaterial(size_t submeshLocalIndex) const
        {
            if (submeshLocalIndex >= m_submeshesMaterials.size())
            {
                return nullptr;
            }
            return m_submeshesMaterials[submeshLocalIndex];
        }

        std::shared_ptr<const Model> getModel() const { return m_model; }
        std::shared_ptr<Model> getModel() { return m_model; }

        void bindModel()    const { m_model->bind(); }
    };

}