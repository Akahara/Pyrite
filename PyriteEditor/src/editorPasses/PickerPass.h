#pragma once

#include "display/RenderGraph/RenderPass.h"
#include "display/GraphicalResource.h"
#include "world/Mesh/RawMeshData.h"
#include "world/Mesh/StaticMesh.h"

#include "editor/EditorActor.h"


namespace pye
{

    namespace EditorPasses
    {
        class PickerPass : public pyr::RenderPass
        {
        private:

            pyr::GraphicalResourceRegistry m_registry;

            // -- ID
            using ActorPickerIDBuffer = pyr::ConstantBuffer<InlineStruct(uint32_t id;) > ;
            std::shared_ptr<ActorPickerIDBuffer> pIdBuffer = std::make_shared<ActorPickerIDBuffer>();

            // -- MVP
            using ActorBuffer = pyr::ConstantBuffer<InlineStruct(mat4 modelMatrix) >;
            std::shared_ptr<ActorBuffer> pActorBuffer = std::make_shared<ActorBuffer>();

            // -- Goal : output a 1,1 texture, called once on click
            pyr::FrameBuffer m_idTarget{ 1,1, pyr::FrameBuffer::COLOR_0 };
            pyr::Effect* m_effect = nullptr;


            std::vector<pye::EditorActor> m_editorActors;
            std::vector<pyr::Actor> m_sceneActors;

        public:

            PickerPass()
            {
                displayName = "Editor-PickerPass";
                m_effect = m_registry.loadEffect(
                    L"res/shaders/depthOnly.fx",
                    pyr::InputLayout::MakeLayoutFromVertex<pyr::RawMeshData::mesh_vertex_t>()
                );

                producesResource("pickerIdBuffer", m_idTarget.getTargetAsTexture(pyr::FrameBuffer::COLOR_0));
            }

            virtual void apply() override
            {
                // Render all objects to a depth only texture

                m_idTarget.clearTargets();
                m_idTarget.bind();

                //for (const pyr::Actor* actor : m_sceneActors)
                //{
                //
                //    smesh->bindModel();
                //
                //    pActorBuffer->setData(ActorBuffer::data_t{ .modelMatrix = smesh->getTransform().getWorldMatrix() });
                //    m_depthOnlyEffect->bindConstantBuffer("ActorBuffer", pActorBuffer);
                //    m_depthOnlyEffect->bind();
                //
                //    std::span<const SubMesh> submeshes = smesh->getModel()->getRawMeshData()->getSubmeshes();
                //    for (auto& submesh : submeshes)
                //    {
                //        Engine::d3dcontext().DrawIndexed(static_cast<UINT>(submesh.getIndexCount()), submesh.startIndex, 0);
                //    }
                //
                //    m_depthOnlyEffect->unbindResources();
                //}
                //
                //m_depthTarget.unbind();

            }
        };
    }
}
