#pragma once
#include "display/Shader.h"
#include "display/GraphicalResource.h"


#include <optional>

namespace pyr
{
    enum class TextureType : INT {
        ALBEDO, NORMAL, BUMP,
        HEIGHT, ROUGHNESS, METALNESS,
        SPECULAR, AO,
        __COUNT,
    };

    struct MaterialMetadata
    {
        // might be worth having multiple textures later, idk
        std::unordered_map<TextureType, std::string> paths
        {
            {TextureType::ALBEDO, {}},
            {TextureType::NORMAL, {}},
            {TextureType::BUMP, {}},
            {TextureType::HEIGHT, {}},
            {TextureType::ROUGHNESS, {}},
            {TextureType::METALNESS, {}},
            {TextureType::SPECULAR, {}},
            {TextureType::AO, {}},
        };

        std::string materialName;

    };



    

    
class Material
{


public:


private:

    // should have a bunch of textures here
    // todo rename, remove bind() operations

    const Effect* m_shader;

    GraphicalResourceRegistry m_grr;
    std::unordered_map<TextureType, Texture> m_textures;

public:

    std::string publicName;

    Material() = default;
    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;
    Material(const Effect* shader) : m_shader(shader)
    {}

    void loadMaterialFromMetadata(const MaterialMetadata& meta)
    {
        publicName = meta.materialName;
        for (TextureType type = TextureType::ALBEDO; type < TextureType::__COUNT; (*(int*)&type)++)
            if (!meta.paths.at(type).empty())
                m_textures[type] = m_grr.loadTexture(string2widestring(meta.paths.at(type)));
    }

    void bind() const  {
        m_shader->bind();
        m_shader->uploadAllBindings();
    }

    
    Texture* getTexture(TextureType type) 
    {
        return m_textures.contains(type) ? &m_textures.at(type) : nullptr;
    }

    const Effect* getEffect() const { return m_shader; }


};

}