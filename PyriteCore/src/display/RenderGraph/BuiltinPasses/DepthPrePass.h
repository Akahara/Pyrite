#pragma once

#include "display/RenderGraph/RenderPass.h"
#include "display/RenderGraph/RenderGraph.h"
#include "display/GraphicalResource.h"
#include "world/Mesh/RawMeshData.h"
#include "world/Mesh/StaticMesh.h"
#include "display/FrameBuffer.h"

namespace pyr
{

    namespace BuiltinPasses
    {

        class DepthPrePass : public RenderPass
        {
        private:

            pyr::GraphicalResourceRegistry m_registry;

            using ActorBuffer = ConstantBuffer < InlineStruct(mat4 modelMatrix) >;
            std::shared_ptr<ActorBuffer> pActorBuffer = std::make_shared<ActorBuffer>();
            
            // goal output a depth texture 
            FrameBuffer m_depthTarget{ Device::getWinWidth(), Device::getWinHeight(), FrameBuffer::DEPTH_STENCIL };
            Effect* m_depthOnlyEffect = nullptr;

        public:

            DepthPrePass()
            {
                displayName = "Depth pre-pass";
                m_depthOnlyEffect = m_registry.loadEffect(
                        L"res/shaders/depthOnly.fx", 
                        InputLayout::MakeLayoutFromVertex<pyr::RawMeshData::mesh_vertex_t>()
                );

                producesResource("depthBuffer", m_depthTarget.getTargetAsTexture(FrameBuffer::DEPTH_STENCIL));
            }

            virtual void apply() override
            {
                if (!PYR_ENSURE(owner)) return;

                // Render all objects to a depth only texture
                m_depthTarget.clearTargets();
                m_depthTarget.bind();

                for (const StaticMesh* smesh : owner->GetContext().ActorsToRender.meshes)
                {

                    smesh->bindModel();

                    pActorBuffer->setData(ActorBuffer::data_t{ .modelMatrix = smesh->GetTransform().getWorldMatrix() });
                    m_depthOnlyEffect->bindConstantBuffer("ActorBuffer", pActorBuffer);
                    m_depthOnlyEffect->bind();
                    
                    std::span<const SubMesh> submeshes = smesh->getModel()->getRawMeshData()->getSubmeshes();
                    for (auto& submesh : submeshes)
                    {
                        Engine::d3dcontext().DrawIndexed(static_cast<UINT>(submesh.getIndexCount()), submesh.startIndex, 0);
                    }

                    m_depthOnlyEffect->unbindResources();
                }

                m_depthTarget.unbind();

            }

            const Effect* getDepthPassEffect() const noexcept { return m_depthOnlyEffect; }
            Texture getOutputDepth() { return m_depthTarget.getTargetAsTexture(FrameBuffer::Target::DEPTH_STENCIL); }

        };
    }
}
