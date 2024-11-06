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
    pyr::Camera* boundCamera = nullptr;

    ForwardPass()
    {
        displayName = "Forward pass";
        
        static constexpr wchar_t DEFAULT_SKYBOX_TEXTURE[] = L"res/textures/pbr/testhdr.dds"; // todo avoid this as the core engine does not have runtime textures
        loadSkybox(DEFAULT_SKYBOX_TEXTURE);
    }

    virtual void apply() override
    {
        PYR_ENSURE(owner);
        if (!PYR_ENSURE(boundCamera)) return;
        pcameraBuffer->setData(CameraBuffer::data_t{ .mvp = boundCamera->getViewProjectionMatrix(), .pos = boundCamera->getPosition() });

        Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTONLY_DEPTH);

        pyr::FrameBuffer::getActiveFrameBuffer().setDepthOverride(m_inputs.at("depthBuffer").res.toDepthStencilView()); // < make sure this input is linked in the scene rdg

        // -- Get all the lights in the context, and bind them
        const pyr::LightsCollections& lights = owner->GetContext().ActorsToRender.lights;
        auto castsShadows = [](pyr::BaseLight* light) -> bool { return light->isOn && light->shadowMode == pyr::DynamicShadow; };
        int shadow_map_index = 0;
        // don't use vectors here... don't duplicate code here.... why am i doing this
        std::vector<Texture> lightmaps_2D{};
        std::vector<Cubemap> lightmaps_3D{};
        static TextureArray lightmaps_2DArray{ 512,512, 8, true };
        static TextureArray lightmaps_3DArray{ 512,512, 8, true, true};

        std::ranges::for_each(owner->GetContext().ActorsToRender.lights.Points, [&castsShadows, &shadow_map_index, &lightmaps_3D, this](pyr::PointLight& light) {
            if (castsShadows(&light))
            {
                pyr::Cubemap lightmap = pyr::SceneRenderTools::MakeSceneDepthCubemapFromPoint(owner->GetContext().ActorsToRender, light.GetTransform().position, 512);
                light.shadowMapIndex = 16 + (lightmaps_3D.size());
                lightmaps_3D.push_back(lightmap);
            }
        });

        std::ranges::for_each(owner->GetContext().ActorsToRender.lights.Spots, [&castsShadows, &shadow_map_index, &lightmaps_2D, this](pyr::SpotLight& light) {
            if (castsShadows(&light))
            {
                pyr::Camera camera{};
                camera.setProjection(light.shadow_projection);
                camera.setPosition(light.GetTransform().position);
                vec3 fuck = { light.GetTransform().rotation.x, light.GetTransform().rotation.y, light.GetTransform().rotation.z };
                camera.lookAt(light.GetTransform().position + fuck);
                pyr::Texture lightmap = pyr::SceneRenderTools::MakeSceneDepth(owner->GetContext().ActorsToRender, camera);
                light.shadowMapIndex = (shadow_map_index++);
                lightmaps_2D.push_back(lightmap);
            }
            });

        std::ranges::for_each(owner->GetContext().ActorsToRender.lights.Directionals, [&castsShadows, &shadow_map_index, &lightmaps_2D, this](pyr::DirectionalLight& light) {
            if (castsShadows(&light))
            {
                pyr::Camera camera{};
                camera.setProjection(light.shadow_projection);
                camera.setPosition(light.GetTransform().position);
                vec3 fuck = { light.GetTransform().rotation.x, light.GetTransform().rotation.y, light.GetTransform().rotation.z };
                camera.lookAt(light.GetTransform().position + fuck);
                camera.rotate(XM_PIDIV2, 0, 0);
                auto test = camera.getViewProjectionMatrix();
                pyr::Texture lightmap = pyr::SceneRenderTools::MakeSceneDepth(owner->GetContext().ActorsToRender, camera);
                light.shadowMapIndex = (shadow_map_index++);
                lightmaps_2D.push_back(lightmap);
            }
        });

        TextureArray::CopyToTextureArray(lightmaps_2D, lightmaps_2DArray);
        TextureArray::CopyToTextureArray(lightmaps_3D, lightmaps_3DArray);

        {
        ImGui::Begin("Debug lightmaps");

        for (auto& lightmap : lightmaps_2D)
        {
            ImGui::Image((void*)lightmap.getRawTexture(), ImVec2{ 256,256 });
        }

        ImGui::End();
        }

        LightsBuffer::data_t light_data{};
        std::copy_n(owner->GetContext().ActorsToRender.lights.ConvertCollectionToHLSL().begin(), std::size(light_data.lights), std::begin(light_data.lights));

        pLightBuffer->setData(light_data);

        // -- Render all objects 
        for (const StaticMesh* mesh : owner->GetContext().ActorsToRender.meshes)
        {
            mesh->bindModel();
            pActorBuffer->setData(ActorBuffer::data_t{ .modelMatrix = mesh->GetTransform().getWorldMatrix() });
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
                effect->bindConstantBuffer("lightsBuffer", pLightBuffer);
                effect->bindTexture(lightmaps_2DArray, "testArray");
                effect->bindTexture(lightmaps_3DArray, "testArrayCube");
                effect->bindCubemaps(lightmaps_3D, "Lightmap3D_Array");


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
