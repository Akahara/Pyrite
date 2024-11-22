#pragma once

#include "display/RenderGraph/RenderPass.h"
#include "display/RenderGraph/RenderGraph.h"
#include "display/GraphicalResource.h"
#include "display/FrameBuffer.h"
#include "world/Mesh/RawMeshData.h"
#include "world/camera.h"
#include "display/RenderProfiles.h"
#include "world/Mesh/StaticMesh.h"
#include "world/Lights/Light.h"
#include "world/Shadows/Lightmap.h"
#include "world/Tools/SceneRenderTools.h"
#include "scene/SceneManager.h"

#include <array>

namespace pyr
{

namespace BuiltinPasses
{

class ForwardPass : public RenderPass
{
private:
    GraphicalResourceRegistry m_registry;

    Effect* m_skyboxEffect;

    std::shared_ptr<ActorBuffer>     pActorBuffer = std::make_shared<ActorBuffer>();
    std::shared_ptr<CameraBuffer>    pcameraBuffer = std::make_shared<CameraBuffer>();
    std::shared_ptr<LightsBuffer>    pLightBuffer = std::make_shared<LightsBuffer>();
    
public:

    Cubemap m_skybox;

    ForwardPass()
    {
        displayName = "Forward pass";
        
        static constexpr wchar_t DEFAULT_SKYBOX_TEXTURE[] = L"res/textures/pbr/testhdr.dds"; // todo avoid this as the core engine does not have runtime textures
        loadSkybox(DEFAULT_SKYBOX_TEXTURE);
    }

    virtual void apply() override
    {
        PYR_ENSURE(owner);
        if (!PYR_ENSURE(owner->GetContext().contextCamera)) return;
        pcameraBuffer->setData(CameraBuffer::data_t{ .mvp = owner->GetContext().contextCamera->getViewProjectionMatrix(), .pos = owner->GetContext().contextCamera->getPosition() });

        Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTONLY_DEPTH);

        Texture depthPrePassBuffer = std::get<Texture>(owner->getResourcesManager().fetchResource("depthBuffer"));
        pyr::FrameBuffer::getActiveFrameBuffer().setDepthOverride(depthPrePassBuffer.toDepthStencilView()); // < make sure this input is linked in the scene rdg

        // -- Get all the lights in the context and create shadow map for those who want lol
        pyr::LightsCollections& lights = owner->GetContext().ActorsToRender.lights;
        auto castsShadows = [](pyr::BaseLight* light) -> bool { return light->isOn && light->shadowMode == pyr::DynamicShadow; };
        int shadow_map_index = 0;

        // don't use vectors here... don't duplicate code here.... why am i doing this
        std::vector<Texture> lightmaps_2D{};
        std::vector<Cubemap> lightmaps_3D{};

        static std::array<pyr::FrameBuffer, 16> lightmaps_2D_fbos{};
        static std::array<pyr::CubemapFramebuffer, 16> lightmaps_3D_fbos{};
        static auto constructLightmaps = [&]() {
            std::ranges::generate(lightmaps_2D_fbos, []() { return FrameBuffer{ 512, 512, FrameBuffer::DEPTH_STENCIL }; });
            std::ranges::generate(lightmaps_3D_fbos, []() { return CubemapFramebuffer{ 512, FrameBuffer::Target::DEPTH_STENCIL | FrameBuffer::COLOR_0 }; });
            return 0;
        }();

        static TextureArray lightmaps_2DArray{ 512,512, 8, TextureArray::Texture2D  , true  };
        static TextureArray lightmaps_3DArray{ 512,512, 8, TextureArray::TextureCube, false };
        static TextureArray empty_Array2D{ 1,1, 1, TextureArray::Texture2D, false };
        static TextureArray empty_Array3D{ 1,1, 1, TextureArray::TextureCube, false };

        if (!lights.Spots.empty() || !lights.Directionals.empty())
        {
            for (int i = 0; i < 16; i++)
            {
                lightmaps_2D_fbos[i].clearTargets();
            }
        }

        std::ranges::for_each(lights.Points, [&](pyr::PointLight& light) {
            if (castsShadows(&light))
            {
                pyr::Cubemap lightmap = pyr::SceneRenderTools::MakeSceneDepthCubemapFromPoint(owner->GetContext().ActorsToRender, light.GetTransform().position, lightmaps_3D_fbos[lightmaps_3D.size()]);
                light.shadowMapIndex = 16 + static_cast<int>((lightmaps_3D.size()));
                lightmaps_3D.push_back(lightmap);
            }
        });

        std::ranges::for_each(lights.Spots, [&](pyr::SpotLight& light) {
            if (castsShadows(&light))
            {
                pyr::Camera camera{};
                camera.setProjection(light.shadow_projection);
                camera.setPosition(light.GetTransform().position);
                vec3 fuck = { light.GetTransform().rotation.x, light.GetTransform().rotation.y, light.GetTransform().rotation.z };
                camera.lookAt(light.GetTransform().position + fuck);
                pyr::Texture lightmap = pyr::SceneRenderTools::MakeSceneDepth(owner->GetContext().ActorsToRender, camera, lightmaps_2D_fbos[shadow_map_index]);
                light.shadowMapIndex = (shadow_map_index++);
                lightmaps_2D.push_back(lightmap);
            }
            });

        std::ranges::for_each(lights.Directionals, [&](pyr::DirectionalLight& light) {
            if (castsShadows(&light))
            {
                pyr::Camera camera{};
                camera.setProjection(light.shadow_projection);
                camera.setPosition(light.GetTransform().position);
                vec3 fuck = { light.GetTransform().rotation.x, light.GetTransform().rotation.y, light.GetTransform().rotation.z };
                camera.lookAt(light.GetTransform().position + fuck);
                camera.rotate(XM_PIDIV2, 0, 0);
                pyr::Texture lightmap = pyr::SceneRenderTools::MakeSceneDepth(owner->GetContext().ActorsToRender, camera, lightmaps_2D_fbos[shadow_map_index]);
                light.shadowMapIndex = (shadow_map_index++);
                lightmaps_2D.push_back(lightmap);
            }
        });

        if (!lightmaps_2D.empty())
            TextureArray::CopyToTextureArray(lightmaps_2D, lightmaps_2DArray);
        if (!lightmaps_3D.empty())
            TextureArray::CopyToTextureArray(lightmaps_3D, lightmaps_3DArray);

        // -- Create raw light buffer

        LightsBuffer::data_t light_data{};
        std::copy_n(lights.ConvertCollectionToHLSL().begin(), std::size(light_data.lights), std::begin(light_data.lights));
        pLightBuffer->setData(light_data);

        std::optional<NamedResource::resource_t> GI_CompositeTexture = owner->getResourcesManager().fetchOptionalResource("GI_CompositeIndirectIllumination");


        // -- Render all objects 
        for (const StaticMesh* mesh : owner->GetContext().ActorsToRender.meshes)
        {
            mesh->bindModel();
            pActorBuffer->setData(ActorBuffer::data_t{ .modelMatrix = mesh->GetTransform().getWorldMatrix() });
            std::span<const SubMesh> submeshes = mesh->getModel()->getRawMeshData()->getSubmeshes();

            pyr::Texture depthPrePassBuffer = std::get<pyr::Texture>(owner->getResourcesManager().fetchResource("depthBuffer"));

            std::optional<NamedResource::resource_t> ssaoTexture = owner->getResourcesManager().fetchOptionalResource("ssaoTexture_blurred");
            
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
                effect->bindConstantBuffer("lightsBuffer", pLightBuffer);
                if (!lightmaps_2D.empty())
                    effect->bindTexture(lightmaps_2DArray, "lightmaps_2D");
                if (!lightmaps_3D.empty())
                    effect->bindTexture(lightmaps_3DArray, "lightmaps_3D");


                if (ssaoTexture) effect->bindTexture(std::get<Texture>(ssaoTexture.value()), "ssaoTexture");
                else effect->bindTexture(pyr::Texture::getDefaultTextureSet().WhitePixel , "ssaoTexture");

                if (GI_CompositeTexture) effect->bindTexture(std::get<Texture>(GI_CompositeTexture.value()), "GI_CompositeTexture");
                else effect->bindTexture(pyr::Texture::getDefaultTextureSet().BlackPixel, "GI_CompositeTexture");

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
