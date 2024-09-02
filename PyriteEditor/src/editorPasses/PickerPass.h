#pragma once

#include "display/RenderGraph/RenderPass.h"
#include "display/GraphicalResource.h"
#include "world/Mesh/RawMeshData.h"
#include "world/Mesh/StaticMesh.h"
#include "inputs/UserInputs.h"

#include "editor/EditorActor.h"
#include "editor/bridges/pf_StaticMesh.h"
#include "editor/Editor.h"

#include "imguizmo/ImGuizmo.h"

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

            using BillboardPickerIDBuffer = pyr::ConstantBuffer<InlineStruct(uint32_t ids[64]) >;
            std::shared_ptr<BillboardPickerIDBuffer> pBillboardIDBuffer = std::make_shared<BillboardPickerIDBuffer>();

            // -- MVP
            using ActorBuffer = pyr::ConstantBuffer<InlineStruct(mat4 modelMatrix) >;
            std::shared_ptr<ActorBuffer> pActorBuffer = std::make_shared<ActorBuffer>();
            using CameraBuffer = pyr::ConstantBuffer < InlineStruct(mat4 mvp; alignas(16) vec3 pos) > ;
            std::shared_ptr<CameraBuffer>           pcameraBuffer = std::make_shared<CameraBuffer>();

            // -- Goal : output a 1,1 texture, called once on click
            pyr::FrameBuffer m_idTarget{ pyr::Device::getWinWidth(),pyr::Device::getWinHeight(), pyr::FrameBuffer::COLOR_0};
            pyr::FrameBuffer m_OutlineTarget{ pyr::Device::getWinWidth(),pyr::Device::getWinHeight(), pyr::FrameBuffer::COLOR_0};

            pyr::Effect* m_pickEffect_Meshes = nullptr;
            pyr::Effect* m_pickEffect_Billboards = nullptr;
            pyr::Effect* m_gridDepthEffect = nullptr;
            pyr::Effect* m_outlineEffect = nullptr;
            pyr::Effect* m_composeEffect = nullptr;


            std::vector<pye::EditorActor> m_editorActors;
            std::vector<pyr::Actor> m_sceneActors;

            pyr::ScreenPoint m_requestedMousePosition;

            bool bShouldPick = false;

            pyr::FrameBuffer m_target{pyr::Device::getWinWidth(), pyr::Device::getWinHeight(), pyr::FrameBuffer::COLOR_0 | pyr::FrameBuffer::DEPTH_STENCIL };
            pyr::FrameBuffer m_targetNoDepth{pyr::Device::getWinWidth(), pyr::Device::getWinHeight(), pyr::FrameBuffer::COLOR_0 };

        public:

            EditorActor* Selected = nullptr;

            std::vector<EditorActor*> selectedActors;

            pyr::Camera* boundCamera = nullptr;

            PickerPass()
            {
                displayName = "Editor-PickerPass";
                m_pickEffect_Meshes = m_registry.loadEffect(
                    L"editor/shaders/picker.fx",
                    pyr::InputLayout::MakeLayoutFromVertex<pyr::RawMeshData::mesh_vertex_t>(),
                    { pyr::Effect::define_t{ .name = "USE_MESH", .value = "1" }}
                );

                m_pickEffect_Billboards = m_registry.loadEffect(
                    L"editor/shaders/picker.fx",
                    pyr::InputLayout::MakeLayoutFromVertex<pyr::EmptyVertex,pyr::Billboard::billboard_vertex_t>(),
                    { pyr::Effect::define_t{ .name = "USE_BILLBOARDS", .value = "1" }}
                );

                m_gridDepthEffect = m_registry.loadEffect(
                    L"editor/shaders/selectionDepthEffect.fx",
                    pyr::InputLayout::MakeLayoutFromVertex<pyr::RawMeshData::mesh_vertex_t>()
                );

                m_outlineEffect = m_registry.loadEffect(
                    L"editor/shaders/selectionOutline.fx",
                    pyr::InputLayout::MakeLayoutFromVertex<pyr::EmptyVertex>()
                );

                m_composeEffect = m_registry.loadEffect(
                    L"editor/shaders/composePick.fx",
                    pyr::InputLayout::MakeLayoutFromVertex<pyr::EmptyVertex>()
                );

                producesResource("pickerIdBuffer", m_idTarget.getTargetAsTexture(pyr::FrameBuffer::COLOR_0));
            }

            virtual void apply() override
            {
                if (!PYR_ENSURE(owner)) return;
                if (!PYR_ENSURE(boundCamera)) return;
                // Render all objects to a depth only texture

                if (bShouldPick)
                {
                    computeSelectedActors();
                    bShouldPick = false;
                }

                if (selectedActors.size() > 0)
                {
                    m_target.clearTargets();
                    m_target.bind();

                    // -- 1 . Depth grid effect and selected meshes depth buffer
                    pyr::RenderProfiles::pushBlendProfile(pyr::BlendProfile::BLEND);

                    m_gridDepthEffect->bindTexture(m_inputs["depthBuffer"].res, "depthBuffer");
                    m_gridDepthEffect->bindConstantBuffer("CameraBuffer", pcameraBuffer);

                    for (EditorActor* actor : selectedActors)
                    {
                        pye::pf_StaticMesh* sm = dynamic_cast<pye::pf_StaticMesh*>(actor);
                        if (!sm) continue;

                        sm->sourceMesh->bindModel();
                        pcameraBuffer->setData(CameraBuffer::data_t{ .mvp = boundCamera->getViewProjectionMatrix(), .pos = boundCamera->getPosition() });
                        pActorBuffer->setData(ActorBuffer::data_t{ .modelMatrix = sm->sourceMesh->getTransform().getWorldMatrix() });
                        m_gridDepthEffect->bindConstantBuffer("ActorBuffer", pActorBuffer);
                        m_gridDepthEffect->bind();
                        std::span<const pyr::SubMesh> submeshes = sm->sourceMesh->getModel()->getRawMeshData()->getSubmeshes();
                        for (auto& submesh : submeshes)
                        {
                            pyr::Engine::d3dcontext().DrawIndexed(static_cast<UINT>(submesh.getIndexCount()), submesh.startIndex, 0);
                        }
                        m_gridDepthEffect->unbindResources();
                    }

                    // -- 2 . Compute outline
                    m_target.unbind();
                    pyr::Texture selectedMeshesDepth = m_target.getTargetAsTexture(pyr::FrameBuffer::DEPTH_STENCIL);
                    m_targetNoDepth.clearTargets();
                    m_targetNoDepth.bind();
                    m_outlineEffect->bindTexture(selectedMeshesDepth, "selectedMeshesDepth");
                    m_outlineEffect->bindTexture(m_inputs["depthBuffer"].res, "sceneDepth");
                    m_outlineEffect->bind();
                    pyr::Engine::d3dcontext().Draw(3, 0);
                    m_outlineEffect->unbindResources();
                    m_targetNoDepth.unbind();

                    // -- 3. Compose scene, outline and depthgrid

                    m_composeEffect->bindTexture(m_targetNoDepth.getTargetAsTexture(pyr::FrameBuffer::COLOR_0), "outlineTexture");
                    m_composeEffect->bindTexture(m_target.getTargetAsTexture(pyr::FrameBuffer::COLOR_0), "depthGridTexture");
                    m_composeEffect->bind();
                    pyr::Engine::d3dcontext().Draw(3, 0);
                    m_composeEffect->unbindResources();
                    pyr::RenderProfiles::popBlendProfile();

                    // -- 4. Guizmos
                    std::vector<pyr::StaticMesh*> meshes;
                    for (EditorActor* actor : selectedActors)
                    {
                        pye::pf_StaticMesh* sm = dynamic_cast<pye::pf_StaticMesh*>(actor);
                        if (sm) meshes.push_back(sm->sourceMesh);
                    }
                    
                    EditTransforms(*boundCamera, meshes);
                }
            }

            void RequestPick()
            {
                bShouldPick = true;
            }
        
        private:

            void computeSelectedActors()
            {

                bool bIsHoldingControl = pyr::UserInputs::isKeyPressed(keys::SC_LEFT_CTRL);

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

                    m_pickEffect_Meshes->bindConstantBuffer("ActorPickerIDBuffer", pIdBuffer);
                    m_pickEffect_Meshes->bindConstantBuffer("CameraBuffer", pcameraBuffer);
                    m_pickEffect_Meshes->bindConstantBuffer("ActorBuffer", pActorBuffer);

                    m_pickEffect_Meshes->bind();

                    std::span<const pyr::SubMesh> submeshes = smesh->getModel()->getRawMeshData()->getSubmeshes();
                    for (auto& submesh : submeshes)
                    {
                        pyr::Engine::d3dcontext().DrawIndexed(static_cast<UINT>(submesh.getIndexCount()), submesh.startIndex, 0);
                    }

                    m_pickEffect_Meshes->unbindResources();
                }

                // -- Billboards
                {
                    pyr::BillboardManager::BillboardsRenderData renderData = pyr::BillboardManager::makeContext(owner->GetContext().ActorsToRender.billboards);
                    std::array<uint32_t, 64> ids;

                    ids[0] = 727;
                    pBillboardIDBuffer->setData(
                        BillboardPickerIDBuffer::data_t{ .ids = ids[0] });
                    
                    m_pickEffect_Billboards->bindConstantBuffer("CameraBuffer", pcameraBuffer);
                    m_pickEffect_Billboards->bindConstantBuffer("BillboardPickerIDBuffer", pBillboardIDBuffer);

                    auto result = renderData.textures
                        | std::views::keys
                        | std::views::transform([](auto texPtr) { return *texPtr; });

                    // Collect the view into a vector
                    std::vector<pyr::Texture> textures(result.begin(), result.end());
                    m_pickEffect_Billboards->bindTextures(textures, "textures");

                    m_pickEffect_Billboards->bind();
                    renderData.instanceBuffer.bind(true);
                    pyr::Engine::d3dcontext().DrawInstanced(6, static_cast<UINT>(renderData.instanceBuffer.getVerticesCount()), 0, 0);
                    m_pickEffect_Billboards->unbindResources();
                }

                m_idTarget.unbind();

                pyr::RenderProfiles::popDepthProfile();

                int actorId = readback();
                if (actorId != ID_NONE)
                {
                    // You have picked actor !!!
                    PYR_LOGF(LogRenderPass, INFO, "Actor {} was picked.", actorId);
                    handlePick(actorId, bIsHoldingControl);

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

            void handlePick(int actorId, bool bPushNewActor = false)
            {
                bool bIsActorRegistered = pye::Editor::Get().RegisteredActors.contains(actorId);
                if (bIsActorRegistered)
                {
                    pye::EditorActor* editorActor = pye::Editor::Get().RegisteredActors.at(actorId);
                    if (!bPushNewActor)
                    {
                        selectedActors.clear();
                    }
                    selectedActors.push_back(editorActor);
                }
                else
                {
                    selectedActors.clear();
                }

            }

            void EditTransforms(const pyr::Camera& camera, std::vector<pyr::StaticMesh*> meshes)
            {
                // Get center of mass ? 
                if (meshes.empty()) return;

                Transform center;
                for (pyr::StaticMesh* mesh : meshes)
                {
                    center.position += mesh->getTransform().position;
                }
                center.position /= meshes.size();
                center.scale = { 1,1,1 };
                center.rotation = { 0,0,0 };


                vec3 originalPos = center.position;
                mat4 matrix = center.getWorldMatrix();

                static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
                static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);

                if (pyr::UserInputs::isKeyPressed(keys::SC_Z)) mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
                if (pyr::UserInputs::isKeyPressed(keys::SC_E)) mCurrentGizmoOperation = ImGuizmo::SCALE;
                if (pyr::UserInputs::isKeyPressed(keys::SC_R)) mCurrentGizmoOperation = ImGuizmo::ROTATE;

                ImGuiIO& io = ImGui::GetIO();
                ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
                if (ImGuizmo::Manipulate(&camera.getViewMatrix()._11, &camera.getProjectionMatrix()._11, mCurrentGizmoOperation, mCurrentGizmoMode, &matrix._11, NULL, NULL))
                {
                    vec3 dScale, dPos;  quat dRot;
                    matrix.Decompose(dScale, dRot, dPos);

                    dPos -= originalPos;
                    for (pyr::StaticMesh* mesh : meshes)
                    {
                        mesh->getTransform().position += dPos;
                        if (meshes.size() > 1)
                        {
                            vec3 rotationPoint = center.position;
                            vec3 diff = mesh->getTransform().position - rotationPoint;
                            vec3 out = (diff * dRot);
                            mesh->getTransform().position += out;
                        }

                        if (mCurrentGizmoOperation == ImGuizmo::SCALE) mesh->getTransform().scale = dScale; // temp
                        mesh->getTransform().rotation *= dRot;
                    }
                }
            }

            void EditTransform(const pyr::Camera& camera, pyr::StaticMesh& mesh)
            {
                static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
                static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);

                mat4 matrix = mesh.getTransform().getWorldMatrix();

                ImGuiIO& io = ImGui::GetIO();
                ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
                ImGuizmo::Manipulate(&camera.getViewMatrix()._11, &camera.getProjectionMatrix()._11, mCurrentGizmoOperation, mCurrentGizmoMode, &matrix._11, NULL, NULL);
                matrix.Decompose(mesh.getTransform().scale, mesh.getTransform().rotation, mesh.getTransform().position);
            }

        private:

            template<class T>
            ID3D11ShaderResourceView* createStructuredBuffer(const std::span<T>& sourceData)
            {
                ID3D11Buffer* buffer = nullptr;
                uint32_t stride = sizeof(T);
                const uint32_t byteWidth = stride * static_cast<uint32_t>(sourceData.size());

                D3D11_BUFFER_DESC desc;
                desc.ByteWidth = byteWidth;
                desc.Usage = D3D11_USAGE_DYNAMIC;
                desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
                desc.StructureByteStride = stride;

                D3D11_SUBRESOURCE_DATA data;
                data.pSysMem = sourceData.data();
                data.SysMemPitch = 0;
                data.SysMemSlicePitch = 0;

                auto hr = pyr::Engine::d3ddevice().CreateBuffer(&desc, &data, &buffer);
                PYR_ENSURE(hr == S_OK);

                D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Format = DXGI_FORMAT_UNKNOWN;
                srvDesc.Buffer.NumElements = sourceData.size(); 
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;

                ID3D11ShaderResourceView* shaderResourceView = nullptr;
                hr = pyr::Engine::d3ddevice().CreateShaderResourceView(buffer, nullptr, &shaderResourceView);
                PYR_ENSURE(hr == S_OK);
                return shaderResourceView;
            }

        };

    }
}
