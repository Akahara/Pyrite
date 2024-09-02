#pragma once

#include "display/RenderGraph/RenderPass.h"
#include "display/RenderGraph/RenderGraph.h"
#include "display/GraphicalResource.h"
#include "display/FrameBuffer.h"
#include "world/Mesh/RawMeshData.h"
#include "world/camera.h"
#include "display/RenderProfiles.h"
#include "world/Mesh/StaticMesh.h"
#include "scene/SceneManager.h"

namespace pyr
{

namespace BuiltinPasses
{

class ForwardPass : public RenderPass
{
private:
    GraphicalResourceRegistry m_registry;

    Effect* m_skyboxEffect;

    using ActorBuffer = ConstantBuffer < InlineStruct(mat4 modelMatrix) >;
    std::shared_ptr<ActorBuffer> pActorBuffer = std::make_shared<ActorBuffer>();
    Effect* m_defaultGGXEffect;

    using CameraBuffer = pyr::ConstantBuffer < InlineStruct(mat4 mvp; alignas(16) vec3 pos) > ;
    std::shared_ptr<CameraBuffer>           pcameraBuffer = std::make_shared<CameraBuffer>();

public:

    Cubemap m_skybox;
    pyr::Camera* boundCamera = nullptr;

    ForwardPass()
    {
        displayName = "Forward";
        
        static constexpr wchar_t DEFAULT_SKYBOX_TEXTURE[] = L"res/textures/pbr/testhdr.dds"; // todo avoid this as the core engine does not have runtime textures
        loadSkybox(DEFAULT_SKYBOX_TEXTURE);
        m_defaultGGXEffect = m_registry.loadEffect(L"res/shaders/ggx.fx", InputLayout::MakeLayoutFromVertex<RawMeshData::mesh_vertex_t>());
    }

    virtual void apply() override
    {
        if (!PYR_ENSURE(boundCamera)) return;
        pcameraBuffer->setData(CameraBuffer::data_t{ .mvp = boundCamera->getViewProjectionMatrix(), .pos = boundCamera->getPosition() });

        Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTONLY_DEPTH);

        pyr::FrameBuffer::getActiveFrameBuffer().setDepthOverride(m_inputs.at("depthBuffer").res.toDepthStencilView()); // < make sure this input is linked in the scene rdg
        // Render all objects 

        for (const StaticMesh* mesh : owner->GetContext().ActorsToRender.meshes)
        {
            mesh->bindModel();
            pActorBuffer->setData(ActorBuffer::data_t{ .modelMatrix = mesh->getTransform().getWorldMatrix() });
            std::span<const SubMesh> submeshes = mesh->getModel()->getRawMeshData()->getSubmeshes();
            std::optional<NamedInput> ssaoTexture = getInputResource("ssaoTexture_blurred");
            
            for (auto& submesh : submeshes)
            {
                //const auto submeshMaterial = pyr::MaterialBank::GetMaterialReference(submesh.materialIndex);
                const auto& submeshMaterial = mesh->getMaterial(submesh.materialIndex);
                if (!submeshMaterial) break; // should not happen because of default mat ?

                const Effect* effect = submeshMaterial->getEffect();
                if (!effect) break;
                
                effect->bindConstantBuffer("CameraBuffer", pcameraBuffer);
                effect->bindConstantBuffer("ActorBuffer", pActorBuffer);
                effect->bindConstantBuffer("ActorMaterials", submeshMaterial->coefsToCbuffer());

                if (ssaoTexture) effect->bindTexture(ssaoTexture.value().res, "ssaoTexture");
                if (submeshMaterial)
                {
                    if (auto tex = submeshMaterial->getTexture(TextureType::ALBEDO); tex)    effect->bindTexture(*tex, "mat_albedo");
                    if (auto tex = submeshMaterial->getTexture(TextureType::NORMAL); tex)    effect->bindTexture(*tex, "mat_normal");
                    if (auto tex = submeshMaterial->getTexture(TextureType::BUMP); tex)      effect->bindTexture(*tex, "mat_normal");
                    if (auto tex = submeshMaterial->getTexture(TextureType::AO); tex)        effect->bindTexture(*tex, "mat_ao");
                    if (auto tex = submeshMaterial->getTexture(TextureType::ROUGHNESS); tex) effect->bindTexture(*tex, "mat_roughness");
                    if (auto tex = submeshMaterial->getTexture(TextureType::METALNESS); tex) effect->bindTexture(*tex, "mat_metalness");
                    if (auto tex = submeshMaterial->getTexture(TextureType::HEIGHT); tex)    effect->bindTexture(*tex,  "mat_height");
                }
                
                effect->bind();
                Engine::d3dcontext().DrawIndexed(static_cast<UINT>(submesh.getIndexCount()), submesh.startIndex, 0);
                effect->unbindResources();
            }
        }
        pyr::RenderProfiles::popDepthProfile();
        renderSkybox();
    }

    void loadSkybox(const GraphicalResourceRegistry::filepath& path)
    {
        static constexpr wchar_t DEFAULT_SKYBOX_SHADER[] = L"res/shaders/skybox.fx";

        m_skybox = m_registry.loadCubemap(path);
        m_skyboxEffect = m_registry.loadEffect(DEFAULT_SKYBOX_SHADER, InputLayout::MakeLayoutFromVertex<EmptyVertex>());
    }

    Effect* getSkyboxEffect() const { return m_skyboxEffect; }
private:

    void renderSkybox()
    {
        assert(boundCamera);
        m_skyboxEffect->bindConstantBuffer("CameraBuffer", pcameraBuffer);
        m_skyboxEffect->bindCubemap(m_skybox, "cubemap");
        RenderProfiles::pushRasterProfile(RasterizerProfile::NOCULL_RASTERIZER);
        RenderProfiles::pushDepthProfile(DepthProfile::TESTWRITE_DEPTH);
        m_skyboxEffect->bind();
        m_skyboxEffect->bindCubemap(m_skybox, "cubemap");
        Engine::d3dcontext().Draw(36, 0);
        m_skyboxEffect->unbindResources();
        RenderProfiles::popDepthProfile();
        RenderProfiles::popRasterProfile();
        pyr::FrameBuffer::getActiveFrameBuffer().setDepthOverride(nullptr);

    }
};
}
}
