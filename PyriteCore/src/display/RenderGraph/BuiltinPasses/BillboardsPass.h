#pragma once

#include "display/RenderGraph/RenderPass.h"
#include "display/RenderGraph/RenderGraph.h"
#include "display/GraphicalResource.h"
#include "world/Mesh/RawMeshData.h"
#include "world/Mesh/StaticMesh.h"
#include "world/Billboards/Billboard.h"
#include "display/FrameBuffer.h"
#include <unordered_set>
#include <ranges>

namespace pyr
{

    namespace BuiltinPasses
    {

        class BillboardsPass : public RenderPass
        {
        private:

            using CameraBuffer = pyr::ConstantBuffer < InlineStruct(mat4 mvp; alignas(16) vec3 pos) > ;
            std::shared_ptr<CameraBuffer>           pcameraBuffer = std::make_shared<CameraBuffer>();

            pyr::GraphicalResourceRegistry m_registry;
            Effect* m_billboardEffect = nullptr;

        public:

            pyr::Camera* boundCamera = nullptr;

            BillboardsPass()
            {
                displayName = "Billboards pass";
                m_billboardEffect = m_registry.loadEffect(
                    L"res/shaders/billboard.fx",
                    InputLayout::MakeLayoutFromVertex<pyr::EmptyVertex, pyr::Billboard::billboard_vertex_t>()
                );

            }

            virtual void apply() override
            {
                if (!PYR_ENSURE(owner)) return;
                if (!PYR_ENSURE(boundCamera)) return;

                if (owner->GetContext().ActorsToRender.billboards.empty()) return;
                
                Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                pyr::RenderProfiles::pushBlendProfile(pyr::BlendProfile::BLEND);
                 
                pcameraBuffer->setData(CameraBuffer::data_t{ .mvp = boundCamera->getViewProjectionMatrix(), .pos = boundCamera->getPosition() });

                // - First step should be to sort the billboards, whether they are HUD (autofacing, no depth test, depth write for the picker) ect
                pyr::BillboardManager::BillboardsRenderData renderData = pyr::BillboardManager::makeContext(owner->GetContext().ActorsToRender.billboards);
                m_billboardEffect->bindConstantBuffer("CameraBuffer", pcameraBuffer);

                // Collect the view into a vector, as the shader array signature is a vector and not a span (should be changed someday)
                std::vector<pyr::Texture> sortedTextures;
                sortedTextures.resize(pyr::BillboardManager::MAX_TEXTURE_COUNT);
                for (const auto& [texPtr, texId] : renderData.textures)
                {
                    if (!texPtr) continue;
                    sortedTextures[texId] = *texPtr;
                }
                m_billboardEffect->bindTextures(sortedTextures, "textures");

                m_billboardEffect->bind();
                renderData.instanceBuffer.bind(true);
                pyr::Engine::d3dcontext().DrawInstanced(6, static_cast<UINT>(renderData.instanceBuffer.getVerticesCount()), 0, 0);
                m_billboardEffect->unbindResources();

                pyr::RenderProfiles::popBlendProfile();
            }
        };
    }
}
