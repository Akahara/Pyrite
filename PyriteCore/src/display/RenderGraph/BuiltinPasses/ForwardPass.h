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

    pyr::GraphicalResourceRegistry m_registry;

    pyr::Cubemap m_skybox;
    pyr::Effect* m_skyboxEffect;

public:

    ForwardPass()
    {
        static constexpr wchar_t DEFAULT_SKYBOX_TEXTURE[] = L"res/textures/skybox.dds"; // todo avoid this as the core engine does not have runtime textures
        loadSkybox(DEFAULT_SKYBOX_TEXTURE);
    }

    virtual void clear() override { m_meshes.clear(); }

    virtual void apply() override
    {
        // Render all objects 
        for (const StaticMesh* smesh : m_meshes)
        {
            smesh->bindModel();
            smesh->bindMaterial();
            // todo bind materials and shaders
            std::span<const SubMesh> subMeshIndices = smesh->getModel()->getRawMeshData()->getSubmeshes();
            for (int index = 0; index < subMeshIndices.size() - 1; ++index)
            {
                const size_t indexCount = subMeshIndices[index + 1].startIndex - subMeshIndices[index].startIndex;
                Engine::d3dcontext().DrawIndexed(static_cast<UINT>(indexCount), subMeshIndices[index].startIndex, 0);
            }
            smesh->getMaterial().getEffect()->unbindResources();
        }

        renderSkybox();
    }

    void loadSkybox(const pyr::GraphicalResourceRegistry::filepath& path)
    {
        static constexpr wchar_t DEFAULT_SKYBOX_SHADER[] = L"res/shaders/skybox.fx";

        m_skybox = m_registry.loadCubemap(path);
        m_skyboxEffect = m_registry.loadEffect(DEFAULT_SKYBOX_SHADER, InputLayout::MakeLayoutFromVertex<EmptyVertex>());
    }

    const pyr::Effect* getSkyboxEffect() const { return m_skyboxEffect; }

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
