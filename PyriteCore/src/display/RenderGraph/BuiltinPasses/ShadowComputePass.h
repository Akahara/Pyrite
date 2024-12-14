#pragma once

#include "display/RenderGraph/RenderPass.h"
#include "display/RenderGraph/RenderGraph.h"
#include "display/GraphicalResource.h"
#include "world/Mesh/RawMeshData.h"
#include "world/Mesh/StaticMesh.h"
#include "world/Tools/CommonConstantBuffers.h"


#include "display/FrameBuffer.h"
#include "world/Tools/SceneRenderTools.h"

namespace pyr
{

    namespace BuiltinPasses
    {

        class ShadowComputePass : public RenderPass
        {
        private:

            enum { LIGHTMAP_MAX_COUNT = 8 };

            pyr::GraphicalResourceRegistry m_registry;

            std::array<pyr::FrameBuffer,        LIGHTMAP_MAX_COUNT> lightmaps_2D_fbos{};
            std::array<pyr::CubemapFramebuffer, LIGHTMAP_MAX_COUNT> lightmaps_3D_fbos{};

            TextureArray lightmaps_2DArray;
            TextureArray lightmaps_3DArray;

        public:

            ShadowComputePass(unsigned int width, unsigned int height) 
                : lightmaps_2DArray{ width,height, LIGHTMAP_MAX_COUNT, TextureArray::Texture2D  , true }
                , lightmaps_3DArray{ width,height, LIGHTMAP_MAX_COUNT, TextureArray::TextureCube, false }
            {
                displayName = "Lightmaps - shadow compute pass";

                std::ranges::generate(lightmaps_2D_fbos, [&]() { return FrameBuffer{ width, height, FrameBuffer::DEPTH_STENCIL }; });
                std::ranges::generate(lightmaps_3D_fbos, [&]() { return CubemapFramebuffer{ width, FrameBuffer::Target::DEPTH_STENCIL | FrameBuffer::COLOR_0 }; });

                producesResource("Lightmaps_2D", lightmaps_2DArray);
                producesResource("Lightmaps_3D", lightmaps_3DArray);
            }

            ShadowComputePass() : ShadowComputePass(512,512) {}

            virtual void apply() override
            {
                // -- Get all the lights in the context and create shadow map for those who want lol
                // NOTE : this is absolutely sub-optimal, as this copies billions of textures around each frame. I definitely need to make TextureArrayFramebuffers or something
                
                pyr::LightsCollections& lights = owner->GetContext().ActorsToRender.lights;
                auto castsShadows = [](pyr::BaseLight* light) -> bool { return light->isOn && light->shadowMode == pyr::DynamicShadow; };
                int shadow_map_index = 0;

                std::vector<Texture> lightmaps_2D{};
                std::vector<Cubemap> lightmaps_3D{};

                if (!lights.Spots.empty() || !lights.Directionals.empty())
                {
                    for (int i = 0; i < LIGHTMAP_MAX_COUNT; i++)
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

                if (!lightmaps_2D.empty()) TextureArray::CopyToTextureArray(lightmaps_2D, lightmaps_2DArray);
                if (!lightmaps_3D.empty()) TextureArray::CopyToTextureArray(lightmaps_3D, lightmaps_3DArray);
            }

        };
    }
}
