#pragma once
#include "display/Shader.h"
#include "display/GraphicalResource.h"
#include "world/Mesh/RawMeshData.h"

#include <filesystem>
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

    struct MaterialRenderingCoefficients
    {
        vec4 Ka = vec4{1.f,.0f,1.f,1.F}; // color
        vec4 Ks = vec4{ 1.f,1.f,1.f,1.F }; // specular
        vec4 Ke; // emissive
        float Roughness = 0.3f; // specular exponent
        float Metallic = 1.F; // specular exponent
        float Ni = 0.04f; // optical density 
        float d = 0.f; // transparency
    };


    using MaterialTexturePathsCollection = std::unordered_map<TextureType, std::string>;


    
class Material
{
public:

private:
    
    const Effect* m_shader = nullptr;
    GraphicalResourceRegistry m_grr;
    std::unordered_map<TextureType, Texture> m_textures;
    MaterialRenderingCoefficients coefs;

public:
    
    using MaterialCoefficientsBuffer = ConstantBuffer<MaterialRenderingCoefficients>;

    std::string d_publicName;
    
    Material() = default;
    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;
    Material(const std::filesystem::path& shaderPath);
    Material(const Effect* shader) : m_shader(shader)
    {}

    static std::shared_ptr<Material> MakeRegisteredMaterial(
        const MaterialTexturePathsCollection& pathsCollection,
        const MaterialRenderingCoefficients& matCoefs = {},
        const std::filesystem::path& renderShaderPath = "res/shaders/ggx.fx",
        std::string name = "UnnamedMaterial");

    static std::shared_ptr<Material> MakeRegisteredMaterial(
        const MaterialTexturePathsCollection& pathsCollection,
        const MaterialRenderingCoefficients& matCoefs = {},
        const Effect* renderShader = nullptr,
        std::string name = "UnnamedMaterial");

    // -- Standard constructor.
    Material(
        const MaterialTexturePathsCollection& pathsCollection,
        const MaterialRenderingCoefficients& matCoefs = {},
        const std::filesystem::path& renderShaderPath = "res/shaders/ggx.fx",
        std::string name = "UnnamedMaterial");

    // -- Version with an effect* already loaded somewhere
    Material(
        const MaterialTexturePathsCollection& pathsCollection,
        const MaterialRenderingCoefficients& matCoefs = {},
        const Effect* renderShader = nullptr, // todo do this correctly, too tired to figure this out
        std::string name = "UnnamedMaterial");
    
public:

    void bind() const  {
        m_shader->bind();
        m_shader->uploadAllBindings();
    }

    Texture* getTexture(TextureType type)  {
        return m_textures.contains(type) ? &m_textures.at(type) : nullptr;
    }
    const Texture* getTexture(TextureType type) const {
        return m_textures.contains(type) ? &m_textures.at(type) : nullptr;
    }

    const Effect* getEffect() const { return m_shader; }
    void setEffect(Effect* shader) { m_shader = shader; }

    // Cbuffer helper, temp
    MaterialCoefficientsBuffer::data_t coefsToData() const 
    {
        return MaterialCoefficientsBuffer::data_t
        {
                .Ka = coefs.Ka , // color
                .Ks = coefs.Ks , // specular
                .Ke = coefs.Ke , // emissive
                .Roughness = coefs.Roughness, // specular exponent
                .Metallic = coefs.Metallic, // specular exponent
                .Ni = coefs.Ni , // optical density 
                .d = coefs.d , // transparency
        };
    }


    const ConstantBuffer<MaterialRenderingCoefficients>& coefsToCbuffer() const
    {
        static auto res = ConstantBuffer<MaterialRenderingCoefficients>{};
        res.setData(coefsToData());
        return res;
    }

    void setMaterialRenderingCoefficients(MaterialRenderingCoefficients inCoefs) { coefs = inCoefs; }
    MaterialRenderingCoefficients getMaterialRenderingCoefficients() const noexcept { return coefs; }
    MaterialRenderingCoefficients& getMaterialRenderingCoefficients() noexcept { return coefs; }

};



// This will hold every single material present at runtime and load shaders
class MaterialBank
{
public:

    using mat_id_t = size_t;

    // We only want a single material that is called unnamedmaterial ? maybe we could generate more names
    // This is called automatically when creating a material ! You should not call this on your own.
    static mat_id_t RegisterMaterial(std::shared_ptr<Material> material, std::string name = "UnnamedMaterial")
    {
        auto& bank = Get();
        size_t nextId = bank.elements.size();
        bank.elements[nextId] = material;
        bank.elementsNames[nextId] = name;
        bank.elementsIds[name] = nextId; // will override each time...
        return nextId;
    }

    static std::shared_ptr<Material> GetMaterialReference(mat_id_t materialGlobalId)
    {
        auto& bank = Get();
        if (!bank.elements.contains(materialGlobalId)) return nullptr;
        return bank.elements[materialGlobalId];
    }

    static std::shared_ptr<Material> GetMaterialReference(const std::string& materialName)
    {
        mat_id_t globalMaterialId = GetMaterialGlobalId(materialName);
        if (globalMaterialId == -1) return nullptr; // todo handle error

        auto& bank = Get();
        return bank.elements[globalMaterialId];
    }

    static std::string GetMaterialName(mat_id_t globalMaterialId)
    {
        auto& bank = Get();
        if (!bank.elementsNames.contains(globalMaterialId)) return "MATERIAL_NOT_FOUND";
        std::string matName = bank.elementsNames[globalMaterialId];
        return matName;
    }

    static mat_id_t GetMaterialGlobalId(const std::string& materialName)
    {
        auto& bank = Get();
        if (!bank.elementsIds.contains(materialName)) return -1;
        mat_id_t globalMaterialId = bank.elementsIds[materialName];
        return globalMaterialId;
    }


    static const Effect* RegisterOrGetCachedShader(const std::filesystem::path& renderShaderPath);

    static const Effect* GetDefaultGGXShader()
    {
        auto& bank = Get();
        static auto defaultGGXShader = bank.m_grr.loadEffect(L"res/shaders/ggx.fx", InputLayout::MakeLayoutFromVertex<RawMeshData::mesh_vertex_t>());
        return defaultGGXShader;
    }

private:

    MaterialBank() = default;
    ~MaterialBank() = default;
    MaterialBank(const MaterialBank&) = delete;
    MaterialBank& operator=(const MaterialBank&) = delete;

    static MaterialBank& Get()
    {
        static MaterialBank bank;
        return bank;
    }

private:

    GraphicalResourceRegistry m_grr;

    std::unordered_map<mat_id_t, std::shared_ptr<Material>> elements;
    std::unordered_map<mat_id_t, std::string> elementsNames;
    std::unordered_map<std::string, mat_id_t> elementsIds; // bad
    std::unordered_map<std::string, const Effect*> cachedRenderShaders;
};






}