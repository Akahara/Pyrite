#pragma once

#include "display/RenderGraph/RenderPass.h"
#include "display/GraphicalResource.h"
#include "world/Mesh/Mesh.h"
#include "world/Mesh/StaticMesh.h"


namespace pyr
{

namespace BuiltinPasses
{

class ForwardPass : public RenderPass
{
private:
    GraphicalResourceRegistry m_registry;

    Cubemap m_skybox;
    Effect* m_skyboxEffect;

    using ActorBuffer = ConstantBuffer < InlineStruct(mat4 modelMatrix) >;

    std::shared_ptr<ActorBuffer> pActorBuffer = std::make_shared<ActorBuffer>();
    Effect* m_defaultGGXEffect;

public:

    ForwardPass()
    {
        displayName = "Forward";
        
        static constexpr wchar_t DEFAULT_SKYBOX_TEXTURE[] = L"res/textures/skybox.dds"; // todo avoid this as the core engine does not have runtime textures
        loadSkybox(DEFAULT_SKYBOX_TEXTURE);
        m_defaultGGXEffect = m_registry.loadEffect(L"res/shaders/ggx.fx", InputLayout::MakeLayoutFromVertex<Mesh::mesh_vertex_t>());
    }

    virtual void apply() override
    {
        // Render all objects 
        for (const StaticMesh* mesh : m_meshes)
        {

            pActorBuffer->setData(ActorBuffer::data_t{ .modelMatrix = mesh->getTransform().getWorldMatrix() });

            std::optional<NamedInput> ssaoTexture = getInputResource("ssaoTexture_blurred");

            // todo bind materials and shaders
            std::span<const SubMesh> submeshes = mesh->getModel()->getRawMeshData()->getSubmeshes();
            auto i = 0;
            for (auto& submesh : submeshes)
            {
                mesh->bindModel();
                const auto& mat = mesh->getMaterialOfIndex(submesh.materialIndex);
                if (!mat) break;
                const Effect* effect = mat->getEffect();  
                if (!effect) effect = m_defaultGGXEffect;
                
                mat->bind();
                effect->bindConstantBuffer("ActorBuffer", pActorBuffer);
                effect->bindConstantBuffer("ActorMaterials", mat->coefsToCbuffer());

                if (ssaoTexture) effect->bindTexture(ssaoTexture.value().res, "ssaoTexture");

                if (i < submeshes.size())
                {

                    auto materialId = submeshes[i++].materialIndex; 
                    const std::shared_ptr<Material>& submeshMaterial = mesh->getMaterialOfIndex(materialId);

                    if (submeshMaterial)
                    {
                        if (auto tex = submeshMaterial->getTexture(TextureType::ALBEDO); tex)    effect->bindTexture(*tex,  "mat_albedo");
                        if (auto tex = submeshMaterial->getTexture(TextureType::NORMAL); tex)    effect->bindTexture(*tex,  "mat_normal");
                        if (auto tex = submeshMaterial->getTexture(TextureType::BUMP); tex)    effect->bindTexture(*tex,  "mat_normal");
                        if (auto tex = submeshMaterial->getTexture(TextureType::AO); tex)        effect->bindTexture(*tex,  "mat_ao");
                        if (auto tex = submeshMaterial->getTexture(TextureType::ROUGHNESS); tex) effect->bindTexture(*tex,  "mat_roughness");
                        if (auto tex = submeshMaterial->getTexture(TextureType::METALNESS); tex) effect->bindTexture(*tex,  "mat_metalness");
                        if (auto tex = submeshMaterial->getTexture(TextureType::HEIGHT); tex)    effect->bindTexture(*tex,  "mat_height");
                    }
                    else
                    {
                        // todo fallback to default material
                    }
                }

                Engine::d3dcontext().DrawIndexed(static_cast<UINT>(submesh.getIndexCount()), submesh.startIndex, 0);
                effect->unbindResources();
            }
            
        }

        renderSkybox();
    }

    void loadSkybox(const GraphicalResourceRegistry::filepath& path)
    {
        static constexpr wchar_t DEFAULT_SKYBOX_SHADER[] = L"res/shaders/skybox.fx";

        m_skybox = m_registry.loadCubemap(path);
        m_skyboxEffect = m_registry.loadEffect(DEFAULT_SKYBOX_SHADER, InputLayout::MakeLayoutFromVertex<EmptyVertex>());
    }

    const Effect* getSkyboxEffect() const { return m_skyboxEffect; }

private:

    void renderSkybox()
    {
        m_skyboxEffect->bind();
        m_skyboxEffect->bindCubemap(m_skybox, "cubemap");
        Engine::d3dcontext().Draw(36, 0);
        m_skyboxEffect->unbindResources();
    }
};
}
}
