#pragma once
#include "display/Shader.h"
#include "display/GraphicalResource.h"


#include <optional>

// todo rename coefs, remove material and give shader to submeshes

namespace pyr
{
    enum class TextureType : INT {
        ALBEDO, NORMAL, BUMP,
        HEIGHT, ROUGHNESS, METALNESS,
        SPECULAR, AO,
        __COUNT,
    };


    struct MaterialCoefs
    {
        vec4 Ka; // color
        vec4 Ks; // specular
        vec4 Ke; // emissive
        float Ns = 0; // specular exponent
        float Ni = 0; // optical density 
        float d = 0; // transparency
        float pqdding = 0; // transparency
        //  int illum; // should be unused for now
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
        MaterialCoefs coefs;
    };

    

    
class Material
{


public:


private:


    const Effect* m_shader;

    GraphicalResourceRegistry m_grr;
    std::unordered_map<TextureType, Texture> m_textures;
    MaterialCoefs coefs;

public:
    
    using MaterialCoefficientsBuffer = ConstantBuffer<MaterialCoefs>;

    std::string publicName;

    Material() = default;
    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;
    Material(const Effect* shader) : m_shader(shader)
    {}

    void loadMaterialFromMetadata(const MaterialMetadata& meta)
    {
        publicName = meta.materialName;
        coefs = meta.coefs;
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

    // Cbuffer helper, temp
    MaterialCoefficientsBuffer::data_t coefsToData() const 
    {
        return MaterialCoefficientsBuffer::data_t
        {
                .Ka = coefs.Ka , // color
                .Ks = coefs.Ks , // specular
                .Ke = coefs.Ke , // emissive
                .Ns = coefs.Ns , // specular exponent
                .Ni = coefs.Ni , // optical density 
                .d = coefs.d , // transparency
        };
    }


    ConstantBuffer<MaterialCoefs> coefsToCbuffer()
    {
        auto res = ConstantBuffer<MaterialCoefs>{};
        res.setData(coefsToData());
        return res;
    }

    void setMaterialCoefs(MaterialCoefs inCoefs) { coefs = inCoefs; }
    MaterialCoefs getMaterialCoefs() const noexcept { return coefs; }

};

}