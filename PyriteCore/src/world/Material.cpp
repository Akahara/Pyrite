#include "Material.h"

#include <filesystem>
#include "Mesh/RawMeshData.h"

#include <memory>

using namespace pyr;

pyr::Material::Material(const std::filesystem::path& shaderPath)
{
    m_shader = m_grr.loadEffect(shaderPath, InputLayout::MakeLayoutFromVertex<pyr::RawMeshData::mesh_vertex_t>());
}

// Code dup

std::shared_ptr<Material> pyr::Material::MakeRegisteredMaterial(const MaterialTexturePathsCollection& pathsCollection, const MaterialRenderingCoefficients& matCoefs, const pyr::Effect* renderShader, std::string name)
{
    std::shared_ptr<pyr::Material> toRegister = std::make_shared<pyr::Material>( pathsCollection, matCoefs, renderShader, name );
    MaterialBank::RegisterMaterial(toRegister, name);
    return toRegister;
}

std::shared_ptr<Material> pyr::Material::MakeRegisteredMaterial(const MaterialTexturePathsCollection& pathsCollection, const MaterialRenderingCoefficients& matCoefs, const std::filesystem::path& renderShaderPath, std::string name)
{
    std::shared_ptr<pyr::Material> toRegister = std::make_shared<pyr::Material>( pathsCollection, matCoefs, renderShaderPath, name );
    MaterialBank::RegisterMaterial(toRegister, name);
    return toRegister;
}

// -- Standard constructor.

pyr::Material::Material(const MaterialTexturePathsCollection& pathsCollection, const MaterialRenderingCoefficients& matCoefs, const std::filesystem::path& renderShaderPath, std::string name)
    :
    Material{
        pathsCollection,
        matCoefs,
        MaterialBank::RegisterOrGetCachedShader(renderShaderPath),
        name
    }

{}

// -- Version with an effect* already loaded somewhere

pyr::Material::Material(
    const MaterialTexturePathsCollection& pathsCollection, 
    const MaterialRenderingCoefficients& matCoefs, 
    const Effect* renderShader, 
    std::string name)
{
    // For each texture type, try to fetch the path and produce a texture (and register it to the grr)
    for (TextureType type = TextureType::ALBEDO; type < TextureType::__COUNT; (*(int*)&type)++)
        if (pathsCollection.contains(type) && !pathsCollection.at(type).empty())
            m_textures[type] = m_grr.loadTexture(string2widestring(pathsCollection.at(type)));
        else
            m_textures[type] = Texture::getDefaultTextureSet().WhitePixel;

    m_shader = renderShader;
    coefs = matCoefs;
    d_publicName = name;
}

const pyr::Effect* pyr::MaterialBank::RegisterOrGetCachedShader(const std::filesystem::path& renderShaderPath)
{
    auto& bank = Get();

    if (bank.cachedRenderShaders.contains(renderShaderPath.string()))
    {
        return bank.cachedRenderShaders[renderShaderPath.string()];
    }

    const Effect* loadedShader = bank.m_grr.loadEffect(renderShaderPath, InputLayout::MakeLayoutFromVertex<RawMeshData::mesh_vertex_t>());
    bank.cachedRenderShaders[renderShaderPath.string()] = loadedShader;
    return loadedShader;
}
