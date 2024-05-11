#pragma once
#include "display/Shader.h"

namespace pyr
{
    
class Material
{

private:

    // should have a bunch of textures here
    // todo rename, remove bind() operations

    const Effect* m_shader;

public:

    Material() = default;
    Material(const Effect* shader) : m_shader(shader)
    {}

    void bind() const  {
        m_shader->bind();
        m_shader->uploadAllBindings();
    }

    const Effect* getEffect() const { return m_shader; }

};

}