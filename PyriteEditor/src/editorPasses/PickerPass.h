#pragma once

#include "display/RenderGraph/RenderPass.h"
#include "display/GraphicalResource.h"
#include "world/Mesh/RawMeshData.h"
#include "world/Mesh/StaticMesh.h"

#include "editor/EditorActor.h"

#include <span>

namespace pye
{

    namespace EditorPasses
    {
        class PickerPass : public pyr::RenderPass
        {
        private:

            pyr::GraphicalResourceRegistry m_registry;

            // -- ID
            using ActorPickerIDBuffer = pyr::ConstantBuffer<InlineStruct( uint32_t id) > ;
            std::shared_ptr<ActorPickerIDBuffer> pIdBuffer = std::make_shared<ActorPickerIDBuffer>();

            // -- MVP
            using ActorBuffer = pyr::ConstantBuffer<InlineStruct(mat4 modelMatrix) >;
            std::shared_ptr<ActorBuffer> pActorBuffer = std::make_shared<ActorBuffer>();
            using CameraBuffer = pyr::ConstantBuffer < InlineStruct(mat4 mvp; alignas(16) vec3 pos) > ;
            std::shared_ptr<CameraBuffer>           pcameraBuffer = std::make_shared<CameraBuffer>();

            // -- Goal : output a 1,1 texture, called once on click
            pyr::FrameBuffer m_idTarget{ pyr::Device::getWinWidth(),pyr::Device::getWinHeight(), pyr::FrameBuffer::COLOR_0};
            pyr::Effect* m_effect = nullptr;


            std::vector<pye::EditorActor> m_editorActors;
            std::vector<pyr::Actor> m_sceneActors;

        public:

            pyr::Camera* boundCamera = nullptr;

            PickerPass()
            {
                m_bIsEnabled = false;
                displayName = "Editor-PickerPass";
                m_effect = m_registry.loadEffect(
                    L"editor/shaders/picker.fx",
                    pyr::InputLayout::MakeLayoutFromVertex<pyr::RawMeshData::mesh_vertex_t>()
                );

                producesResource("pickerIdBuffer", m_idTarget.getTargetAsTexture(pyr::FrameBuffer::COLOR_0));
            }

            virtual void apply() override
            {
                if (!PYR_ENSURE(owner)) return;
                if (!PYR_ENSURE(boundCamera)) return;
                // Render all objects to a depth only texture
                pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTONLY_DEPTH);
                m_idTarget.setDepthOverride(m_inputs["depthBuffer"].res.toDepthStencilView());

                PYR_LOG(LogRenderPass, INFO, "Pick !");
                auto renderDoc = pyr::RenderDoc::Get();
                PYR_LOG(LogRenderPass, INFO, "%p", renderDoc);

                if (renderDoc)
                    renderDoc->StartFrameCapture(nullptr, nullptr);

                m_idTarget.clearTargets();
                m_idTarget.bind();

                pcameraBuffer->setData(CameraBuffer::data_t{ .mvp = boundCamera->getViewProjectionMatrix(), .pos = boundCamera->getPosition() });
                for (const pyr::StaticMesh* smesh : owner->GetContext().ActorsToRender.meshes)
                {
                    smesh->bindModel();
                    pActorBuffer->setData(ActorBuffer::data_t{ .modelMatrix = smesh->getTransform().getWorldMatrix() });
                    pIdBuffer->setData(ActorPickerIDBuffer::data_t{ .id = smesh->GetActorID() });

                    m_effect->bindConstantBuffer("ActorPickerIDBuffer", pIdBuffer);
                    m_effect->bindConstantBuffer("CameraBuffer", pcameraBuffer);
                    m_effect->bindConstantBuffer("ActorBuffer", pActorBuffer);

                    m_effect->bind();
                
                    std::span<const pyr::SubMesh> submeshes = smesh->getModel()->getRawMeshData()->getSubmeshes();
                    for (auto& submesh : submeshes)
                    {
                        pyr::Engine::d3dcontext().DrawIndexed(static_cast<UINT>(submesh.getIndexCount()), submesh.startIndex, 0);
                    }
                
                    m_effect->unbindResources();
                }
                
                m_idTarget.unbind();
                if (renderDoc)
                    renderDoc->EndFrameCapture(nullptr, nullptr);

                pyr::RenderProfiles::popDepthProfile();

            }
        };
    }
}
