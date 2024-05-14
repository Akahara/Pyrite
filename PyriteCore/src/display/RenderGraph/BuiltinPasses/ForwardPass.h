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

public:

    ForwardPass()
    {
        static constexpr wchar_t DEFAULT_SKYBOX_TEXTURE[] = L"res/textures/skybox.dds"; // todo avoid this as the core engine does not have runtime textures
        loadSkybox(DEFAULT_SKYBOX_TEXTURE);
    }

    virtual void apply() override
    {
        // Render all objects 
        for (const StaticMesh* smesh : m_meshes)
        {
            const auto& material = smesh->getMaterial();
            const Effect* effect = material->getEffect();

            pActorBuffer->setData(ActorBuffer::data_t{ .modelMatrix = smesh->getTransform().getWorldMatrix() });

            auto optSSAOTexture = getInputResource("ssaoTexture");
            if (optSSAOTexture)
            {
                effect->bindTexture(optSSAOTexture.value().res, "ssaoTexture");
            }

            // todo bind materials and shaders
            std::span<const SubMesh> submeshes = smesh->getModel()->getRawMeshData()->getSubmeshes();
            auto i = 0;
            for (auto& submesh : submeshes)
            {
                smesh->bindModel();
                smesh->bindMaterial();
                effect->bindConstantBuffer("ActorBuffer", pActorBuffer);

                // why does sponza need this ??? FIND THIS ALBIN
                if (i < submeshes.size()-1)
                {

                    auto materialId = submeshes[++i].materialIndex; 
                    const std::shared_ptr<Material>& submeshMaterial = smesh->getMaterialOfIndex(materialId);

                    if (submeshMaterial)
                    {
                        if (auto tex = submeshMaterial->getTexture(TextureType::ALBEDO); tex) effect->bindTexture(*tex,"mat_albedo");
                        if (auto tex = submeshMaterial->getTexture(TextureType::NORMAL); tex) effect->bindTexture(*tex,"mat_normal");
                    }
                    else
                    {
                        // todo fallback to default material
                    }
                }

                Engine::d3dcontext().DrawIndexed(static_cast<UINT>(submesh.getIndexCount()), submesh.startIndex, 0);
                Effect::unbindResources();
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
