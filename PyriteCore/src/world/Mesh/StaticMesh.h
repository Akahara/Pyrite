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
    Material m_material;


public:

    // render ?

    StaticMesh() = default;
    StaticMesh(
        const Model& model, 
        const Material& material = {}/* add defautl mat here*/) : m_model(&model), m_material(material)
    {}

    const pyr::Model* getModel() const { return m_model; }

    void bindModel() const      { m_model->bind(); }
    void bindMaterial() const   { m_material.bind(); }

};

}