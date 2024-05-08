#pragma once
#include "display/Shader.h"

namespace pyr
{
    
class Material
{

private:

    // should have a bunch of textures here

    const Effect* m_shader;

public:

    Material() = default;
    Material(const Effect* shader) : m_shader(shader)
    {}

    void bind() const 
    {
        m_shader->bind();
    }
};

}