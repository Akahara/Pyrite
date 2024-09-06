#pragma once

#include "display/RenderGraph/RenderPass.h"
#include "display/GraphicalResource.h"
#include "world/Mesh/RawMeshData.h"
#include "world/Mesh/StaticMesh.h"
#include "inputs/UserInputs.h"

#include "editor/EditorActor.h"
#include "editor/Editor.h"

#include <span>

namespace pye
{

    namespace EditorPasses
    {
        class PickerPass : public pyr::RenderPass
        {
        private:

            enum { ID_NONE = -1 };

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

            pyr::ScreenPoint m_requestedMousePosition;


        public:

            EditorActor* Selected = nullptr;
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

                m_requestedMousePosition = pyr::UserInputs::getMousePosition();
                pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTONLY_DEPTH);
                m_idTarget.setDepthOverride(m_inputs["depthBuffer"].res.toDepthStencilView());

                PYR_LOG(LogRenderPass, INFO, "Pick !");
                auto renderDoc = pyr::RenderDoc::Get();
                if (renderDoc)    renderDoc->StartFrameCapture(nullptr, nullptr);
                PYR_LOG(LogRenderPass, INFO, "%p", renderDoc);

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

                pyr::RenderProfiles::popDepthProfile();

                int actorId = readback();
                if (actorId != ID_NONE) 
                {
                    // You have picked actor !!!
                    PYR_LOGF(LogRenderPass, INFO, "Actor {} was picked.", actorId);
                    handlePick(actorId);

                }
                if (renderDoc)
                    renderDoc->EndFrameCapture(nullptr, nullptr);
                
            }


            int readback() {
            
                pyr::Texture sourceTexture = m_idTarget.getTargetAsTexture(pyr::FrameBuffer::COLOR_0);

                D3D11_SHADER_RESOURCE_VIEW_DESC sourceDesc{};
                sourceTexture.getRawTexture()->GetDesc(&sourceDesc);

                // -- Create a staging texture with the format of the source texture
                ID3D11Texture2D* StagingTexture;
                D3D11_TEXTURE2D_DESC srcDesc;
                ZeroMemory(&srcDesc, sizeof(srcDesc));
                srcDesc.Width = 1;
                srcDesc.Height = 1;
                srcDesc.ArraySize = 1;
                srcDesc.MipLevels = 0;
                srcDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
                srcDesc.SampleDesc.Count = 1;
                srcDesc.SampleDesc.Quality = 0;
                srcDesc.Usage = D3D11_USAGE_STAGING;
                srcDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
                srcDesc.BindFlags = 0;
                srcDesc.MiscFlags = 0;
                auto hres = pyr::Engine::d3ddevice().CreateTexture2D(&srcDesc, NULL, &StagingTexture);
                if (hres != S_OK || !StagingTexture)
                {
                    HRESULT failed = pyr::Engine::d3ddevice().GetDeviceRemovedReason();
                    return ID_NONE;
                }

                D3D11_BOX Region;
                Region.left = std::clamp<UINT>(m_requestedMousePosition.x * pyr::Device::getWinHeight(), 0, pyr::Device::getWinWidth() - 1);
                Region.right = Region.left + 1;
                Region.top = pyr::Device::getWinHeight() - std::clamp<UINT>(m_requestedMousePosition.y * pyr::Device::getWinHeight(), 0, pyr::Device::getWinHeight() - 1);
                Region.bottom = Region.top + 1;
                Region.front = 0;
                Region.back = 1;

                // -- Copy the 1x1 clicked pixel 
                pyr::Engine::d3dcontext().CopySubresourceRegion(StagingTexture, 0, 0,0,0, sourceTexture.getRawResource(),0, &Region);

                // -- Map the staging texture for cpu reading
                D3D11_MAPPED_SUBRESOURCE mappedResource{};
                auto hr = pyr::Engine::d3dcontext().Map(StagingTexture, 0, D3D11_MAP_READ, 0, &mappedResource);
                if (hr != S_OK)
                {
                    return ID_NONE;
                }

                uint32_t actorId = 0;
                if (mappedResource.pData) {
                    float* pixelData = static_cast<float*>(mappedResource.pData);
                    float redChannelValue = pixelData[0];
                    actorId = static_cast<uint32_t>(redChannelValue);
                }

                pyr::Engine::d3dcontext().Unmap(StagingTexture, 0);

                if (StagingTexture) {
                    StagingTexture->Release();
                }

                return actorId;
            }

            void handlePick(int actorId)
            {
                if (pye::Editor::Get().RegisteredActors.contains(actorId))
                {
                    pye::EditorActor* editorActor = pye::Editor::Get().RegisteredActors.at(actorId);
                    Selected = editorActor;
                }
                else
                {
                    Selected = nullptr;
                }

            }


        };
    }
}
