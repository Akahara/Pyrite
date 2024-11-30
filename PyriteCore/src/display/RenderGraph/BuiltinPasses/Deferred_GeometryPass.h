#pragma once

#include "display/RenderGraph/RenderPass.h"
#include "display/RenderGraph/RenderGraph.h"
#include "display/GraphicalResource.h"
#include "world/Mesh/RawMeshData.h"
#include "world/Mesh/StaticMesh.h"
#include "world/Tools/CommonConstantBuffers.h"

#include "display/FrameBuffer.h"
#include "display/texture.h"
#include "world/Material.h"

#include <imgui.h>

namespace pyr
{

    namespace BuiltinPasses
    {

        class Deferred_GeometryPass : public RenderPass
        {
        private:

            pyr::GraphicalResourceRegistry m_registry;

            std::shared_ptr<ActorBuffer>        pActorBuffer = std::make_shared<ActorBuffer>();
            std::shared_ptr<pyr::CameraBuffer>  pcameraBuffer = std::make_shared<pyr::CameraBuffer>();

        public:

            enum GeometryBuffer_Layers : uint8_t
            {
                Depth           = pyr::FrameBuffer::DEPTH_STENCIL,
                Albedo          = pyr::FrameBuffer::COLOR_0,
                Normal          = pyr::FrameBuffer::COLOR_1,
                WorldPosition   = pyr::FrameBuffer::COLOR_2,
                ARM             = pyr::FrameBuffer::COLOR_3,
                __COUNT         = 4
            };
            static constexpr uint8_t GeometryLayers = Depth | Albedo | Normal | WorldPosition | ARM ;
            using G_Buffer = TextureArray;
        
        private:

            Effect* m_computeGBufferEffect = nullptr;
            G_Buffer m_gBuffer;
            FrameBuffer m_target;

        public:

            Deferred_GeometryPass(unsigned int width, unsigned int height)
                : m_target{ width , height, GeometryLayers }
                , m_gBuffer{ width, height, GeometryBuffer_Layers::__COUNT, TextureArray::Texture2D }
            {
                displayName = "Deferred Geometry Pass (G_Buffer)";
                m_computeGBufferEffect = m_registry.loadEffect(
                    L"res/shaders/compute_gbuffer.fx",
                    InputLayout::MakeLayoutFromVertex<pyr::RawMeshData::mesh_vertex_t>()
                );

                producesResource("G_Buffer", m_gBuffer);

                m_target.getTargetAsTexture(Albedo).SetDebugName("G_Albedo");
                m_target.getTargetAsTexture(Normal).SetDebugName("G_Normal");
                m_target.getTargetAsTexture(WorldPosition).SetDebugName("G_WorldPosition");
                m_target.getTargetAsTexture(ARM).SetDebugName("G_ARM");
            }

            Deferred_GeometryPass()
                : Deferred_GeometryPass(pyr::Device::getWinWidth(), pyr::Device::getWinHeight())
            {}

            virtual void apply() override
            {
                if (!PYR_ENSURE(owner)) return;
                if (!PYR_ENSURE(owner->GetContext().contextCamera)) return;
                pcameraBuffer->setData(CameraBuffer::data_t{ .mvp = owner->GetContext().contextCamera->getViewProjectionMatrix(), .pos = owner->GetContext().contextCamera->getPosition() });

                // -- Use depth prepass if it exists
                Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                
                std::optional<pyr::NamedResource::resource_t> opt_depthBuffer = owner->getResourcesManager().fetchOptionalResource("depthBuffer");
                if (opt_depthBuffer.has_value())
                {
                    pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTONLY_DEPTH);
                    m_target.setDepthOverride(std::get<pyr::Texture>(*opt_depthBuffer).toDepthStencilView());
                }
                else
                {
                    pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTWRITE_DEPTH);
                }

                m_target.clearTargets();
                m_target.bind();
                m_computeGBufferEffect->bindConstantBuffer("CameraBuffer", pcameraBuffer);

                // -- Render all objects 
                for (const StaticMesh* mesh : owner->GetContext().ActorsToRender.meshes)
                {
                    mesh->bindModel();
                    pActorBuffer->setData(ActorBuffer::data_t{ .modelMatrix = mesh->GetTransform().getWorldMatrix() });
                    std::span<const SubMesh> submeshes = mesh->getModel()->getRawMeshData()->getSubmeshes();
                    m_computeGBufferEffect->bindConstantBuffer("ActorBuffer", pActorBuffer);

                    for (auto& submesh : submeshes)
                    {
                        const auto submeshMaterial = pyr::MaterialBank::GetMaterialReference(submesh.materialIndex);
                        
                        if (submeshMaterial)
                        {
                            m_computeGBufferEffect->bindConstantBuffer("ActorMaterials", submeshMaterial->coefsToCbuffer());
                            if (pyr::Texture* tex = submeshMaterial->getTexture(TextureType::ALBEDO); tex)    m_computeGBufferEffect->bindTexture(*tex, "mat_albedo");
                            if (pyr::Texture* tex = submeshMaterial->getTexture(TextureType::NORMAL); tex)    m_computeGBufferEffect->bindTexture(*tex, "mat_normal");
                            if (pyr::Texture* tex = submeshMaterial->getTexture(TextureType::BUMP); tex)      m_computeGBufferEffect->bindTexture(*tex, "mat_normal");
                            if (pyr::Texture* tex = submeshMaterial->getTexture(TextureType::AO); tex)        m_computeGBufferEffect->bindTexture(*tex, "mat_ao");
                            if (pyr::Texture* tex = submeshMaterial->getTexture(TextureType::ROUGHNESS); tex) m_computeGBufferEffect->bindTexture(*tex, "mat_roughness");
                            if (pyr::Texture* tex = submeshMaterial->getTexture(TextureType::METALNESS); tex) m_computeGBufferEffect->bindTexture(*tex, "mat_metalness");
                            if (pyr::Texture* tex = submeshMaterial->getTexture(TextureType::HEIGHT); tex)    m_computeGBufferEffect->bindTexture(*tex, "mat_height");
                        }

                        m_computeGBufferEffect->bind();
                        Engine::d3dcontext().DrawIndexed(static_cast<UINT>(submesh.getIndexCount()), submesh.startIndex, 0);
                    }
                }

                m_computeGBufferEffect->unbindResources();
                m_target.unbind();
                pyr::RenderProfiles::popDepthProfile();
                
                pyr::TextureArray::CopyToTextureArray(
                    {
                        m_target.getTargetAsTexture(Albedo),
                        m_target.getTargetAsTexture(Normal),
                        m_target.getTargetAsTexture(WorldPosition),
                        m_target.getTargetAsTexture(ARM),
                    }, m_gBuffer);
            }

        };
    }
}
