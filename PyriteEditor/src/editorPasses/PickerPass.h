#pragma once

#include <d3d11.h>
#include <ranges>
#include <span>
#include <memory>

#include "utils/Debug.h"
#include "display/RenderGraph/RenderPass.h"
#include "display/GraphicalResource.h"
#include "world/Mesh/RawMeshData.h"
#include "world/Mesh/StaticMesh.h"
#include "world/Transform.h"
#include "world/camera.h" 
#include "inputs/UserInputs.h"

#include "editor/EditorActor.h"
#include "editor/bridges/pf_StaticMesh.h"
#include "editor/Editor.h"

#include "imguizmo/ImGuizmo.h"



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

            using BillboardPickerIDBuffer = pyr::ConstantBuffer < InlineStruct(struct test { alignas(16) uint32_t id; }; test ids[16]) > ;
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
            pyr::Effect* m_renderBillboardEffect = nullptr;


            std::vector<pye::EditorActor> m_editorActors;
            std::vector<pyr::Actor> m_sceneActors;

            pyr::ScreenPoint m_requestedMousePosition;

            bool bShouldPick = false;

            pyr::FrameBuffer m_target{pyr::Device::getWinWidth(), pyr::Device::getWinHeight(), pyr::FrameBuffer::COLOR_0 | pyr::FrameBuffer::DEPTH_STENCIL };
            pyr::FrameBuffer m_targetNoDepth{pyr::Device::getWinWidth(), pyr::Device::getWinHeight(), pyr::FrameBuffer::COLOR_0 };

            ID3D11Texture2D* StagingTexture;

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

                m_renderBillboardEffect = m_registry.loadEffect(
                    L"res/shaders/billboard.fx",
                    pyr::InputLayout::MakeLayoutFromVertex<pyr::EmptyVertex, pyr::Billboard::billboard_vertex_t>(),
                    { pyr::Effect::define_t{.name = "USE_TEXTURE_AS_DEPTH", .value = "1"}}
                );

                producesResource("pickerIdBuffer", m_idTarget.getTargetAsTexture(pyr::FrameBuffer::COLOR_0));

                // -- Create a staging texture with the format of the source texture

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
                    PYR_ASSERT(false);
                }
            }

            virtual ~PickerPass()
            {
                if (StagingTexture) {
                    StagingTexture->Release();
                }

            }

            virtual void apply() override
            {
                if (!PYR_ENSURE(owner)) return;
                if (!PYR_ENSURE(boundCamera)) return; // should just get the main camera somehow

                if (pyr::UserInputs::consumeClick(pyr::MouseState::BUTTON_PRIMARY) && ImGui::GetIO().WantCaptureMouse == false)
                {
                    computeSelectedActors();
                }

                if (selectedActors.size() > 0)
                {

                    RenderSelectedActors();

                    // -- 4. Guizmos
                    std::vector<Transform*> transformsToEdit;
                    for (EditorActor* actor : selectedActors)
                    {
                        pye::pf_StaticMesh* sm = dynamic_cast<pye::pf_StaticMesh*>(actor);
                        if (sm)
                        {
                            transformsToEdit.push_back(&sm->sourceMesh->GetTransform());
                            continue;
                        }
                        pye::pf_BillboardHUD* bb = dynamic_cast<pye::pf_BillboardHUD*>(actor);
                        if (bb)
                        {
                            transformsToEdit.push_back(&((pyr::PointLight*)bb->coreActor)->GetTransform());
                            continue;
                        }

                    }
                    
                    EditTransforms(*boundCamera, transformsToEdit);
                }
            }

        
        private:

            /// Draws all static meshes and billboards to a 1x1 texture, reads the actor id CPU-side and adds the ID to the selected actor list, pushing on top if CTRL is held.
            /// This method is quite huge and seems to scale poorly, but i don't really know what else than billboards and static meshes are going to be pickable ?
            /// 
            /// TODO : Cycle through actors, not sure how to do this other than excluding the selected actors in the target ?
            void computeSelectedActors()
            {

                bool bIsHoldingControl = pyr::UserInputs::isKeyPressed(keys::SC_LEFT_CTRL);

                m_requestedMousePosition = pyr::UserInputs::getMousePosition();
                pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTONLY_DEPTH);
                m_idTarget.setDepthOverride(m_inputs["depthBuffer"].res.toDepthStencilView());

                auto renderDoc = pyr::RenderDoc::Get();
                if (renderDoc)    renderDoc->StartFrameCapture(nullptr, nullptr);

                m_idTarget.clearTargets();
                m_idTarget.bind();

                pcameraBuffer->setData(CameraBuffer::data_t{ .mvp = boundCamera->getViewProjectionMatrix(), .pos = boundCamera->getPosition() });
                for (const pyr::StaticMesh* smesh : owner->GetContext().ActorsToRender.meshes)
                {
                    // Do not render already selected actors ?
                    // TODO : this could go into an editor setting thing
                    //

                    // TODO : To get this to work properly, we need another depth pass, which would also be correct as the editor pass would come as a standalone injection to a rendergraph
                    //if (std::find_if(selectedActors.begin(), selectedActors.end(), [smesh](pye::EditorActor* selectedActor)
                    //{
                    //        auto mesh = dynamic_cast<pye::pf_StaticMesh*>(selectedActor);
                    //        return mesh && mesh->sourceMesh == smesh;
                    //}) != selectedActors.end())
                    //{
	                //    continue;
                    //}


                    smesh->bindModel();
                    pActorBuffer->setData(ActorBuffer::data_t{ .modelMatrix = smesh->GetTransform().getWorldMatrix() });
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
                auto& Editor = pye::Editor::Get();
                if (!Editor.WorldHUD.empty()) {



                    std::vector<const pyr::Billboard*> bbs;
                    for (auto* editorBB : Editor.WorldHUD)
                    {
                        bbs.push_back(editorBB->editorBillboard);
                    }
                    pyr::BillboardManager::BillboardsRenderData renderData = pyr::BillboardManager::makeContext(bbs);

                    BillboardPickerIDBuffer::data_t data{};
                    for (size_t i = 0; i < bbs.size(); i++)
                        data.ids[i].id = bbs[i]->GetActorID();

                    pBillboardIDBuffer->setData(data);

                	m_pickEffect_Billboards->bindConstantBuffer("CameraBuffer", pcameraBuffer);
                    m_pickEffect_Billboards->bindConstantBuffer("BillboardPickerIDBuffer", pBillboardIDBuffer);

                    std::vector<pyr::Texture> sortedTextures;
                    sortedTextures.resize(16);
                    for (const auto& [texPtr, texId] : renderData.textures)
                    {
                        sortedTextures[texId] = *texPtr;
                    }
                    m_pickEffect_Billboards->bindTextures(sortedTextures, "textures");

                    m_pickEffect_Billboards->bind();
                    renderData.instanceBuffer.bind(true);
                    pyr::Engine::d3dcontext().DrawInstanced(6, static_cast<UINT>(renderData.instanceBuffer.getVerticesCount()), 0, 0);
                    m_pickEffect_Billboards->unbindResources();
                }

                m_idTarget.unbind();

                pyr::RenderProfiles::popDepthProfile();

                int actorId = ReadActorIDFromTexture();
                if (actorId != ID_NONE)
                {
                    // You have picked actor !!!
                    PYR_LOGF(LogRenderPass, INFO, "Actor {} was picked.", actorId);
                    AddSelectedActor(actorId, bIsHoldingControl);

                }
                if (renderDoc)
                    renderDoc->EndFrameCapture(nullptr, nullptr);
            }

            /// Creates a staging texture to readback from the 1x1 target.
            /// Returns the editor actor ID, or ID_NONE (== -1) if the operation fails.
            /// Note that editor ID start at 1, so reading 0 is valid and corresponds to basically no actor.
            int ReadActorIDFromTexture() {

                // -- Get the source texture where we rendered stuff
                pyr::Texture sourceTexture = m_idTarget.getTargetAsTexture(pyr::FrameBuffer::COLOR_0);

                // -- Copy the 1x1 clicked pixel 
                D3D11_BOX Region;
                Region.left = std::clamp<UINT>(
                    UINT(m_requestedMousePosition.x * pyr::Device::getWinHeight()), 0, UINT(pyr::Device::getWinWidth()) - 1);
                Region.right = Region.left + 1;
                Region.top = UINT(pyr::Device::getWinHeight()) - std::clamp<UINT>(UINT(m_requestedMousePosition.y * pyr::Device::getWinHeight())
                    , 0, UINT(pyr::Device::getWinHeight() - 1));
                Region.bottom = Region.top + 1;
                Region.front = 0;
                Region.back = 1;

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


                return actorId;
            }

            /// Gets all editor-side registered actors and check if the requested ID exists, and adds it accordingly to the selectedActor list.
            void AddSelectedActor(int actorId, bool bPushNewActor = false)
            {
                auto& Editor = pye::Editor::Get();
                bool bIsActorRegistered = Editor.RegisteredActors.contains(actorId);
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

            /// !! WARNING !!
            /// 
            /// This currently works very poorly.
            /// 
            /// This works fine for single manipulation, altough the guizmo transform is offset for submeshes.
            /// The scaling and rotation for multiple selection is broken. Albin will fix this ! :wink:
            void EditTransforms(const pyr::Camera& camera, std::vector<Transform*> transforms)
            {
                // Get center of mass ? 
                if (transforms.empty()) return;

                Transform center;
                for (Transform* t : transforms)
                {
                    center.position += t->position;
                }
                center.position /= transforms.size();
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
                    for (Transform* t : transforms)
                    {
                        t->position += dPos;
                        if (transforms.size() > 1)
                        {
                            vec3 rotationPoint = center.position;
                            vec3 diff = t->position - rotationPoint;
                            vec3 out = (diff * dRot);
                            t->position += out;
                        }

                        if (mCurrentGizmoOperation == ImGuizmo::SCALE) t->scale = dScale; // temp
                        t->rotation *= dRot;
                    }
                }
            }

            /// Draws all actors in the selectedActor list with outlines and grid effect (if static meshes).
            /// Could use some improvements.
            void RenderSelectedActors()
            {
                // -- 0 . Clear and update buffers
                m_target.clearTargets();
                m_target.bind();
                pcameraBuffer->setData(CameraBuffer::data_t{ .mvp = boundCamera->getViewProjectionMatrix(), .pos = boundCamera->getPosition() });

                // -- 1 . Depth grid effect and selected meshes depth buffer. We store billboards separatly and don't render the grid (they are always on top for now)
                pyr::RenderProfiles::pushBlendProfile(pyr::BlendProfile::BLEND);

                m_gridDepthEffect->bindTexture(m_inputs["depthBuffer"].res, "depthBuffer");
                m_gridDepthEffect->bindConstantBuffer("CameraBuffer", pcameraBuffer);

                std::vector<const pyr::Billboard*> selectedBillboards; // < dirty thing
                for (EditorActor* actor : selectedActors)
                {
                    pye::pf_StaticMesh* sm = dynamic_cast<pye::pf_StaticMesh*>(actor);
                    if (!sm) {
                        pye::pf_BillboardHUD* hud = dynamic_cast<pye::pf_BillboardHUD*>(actor);
                        if (hud)
                        {
                            selectedBillboards.push_back(hud->editorBillboard);
                        }

                        continue;
                    }

                    sm->sourceMesh->bindModel();
                    pActorBuffer->setData(ActorBuffer::data_t{ .modelMatrix = sm->sourceMesh->GetTransform().getWorldMatrix() });
                    m_gridDepthEffect->bindConstantBuffer("ActorBuffer", pActorBuffer);
                    m_gridDepthEffect->bind();
                    std::span<const pyr::SubMesh> submeshes = sm->sourceMesh->getModel()->getRawMeshData()->getSubmeshes();
                    for (auto& submesh : submeshes)
                    {
                        pyr::Engine::d3dcontext().DrawIndexed(static_cast<UINT>(submesh.getIndexCount()), submesh.startIndex, 0);
                    }
                    m_gridDepthEffect->unbindResources();
                }

                // >>> 1.1 : Render all billboards into the current depth buffer ?

                // Call this using the billboard.fx effect
                if (!selectedBillboards.empty())
                {
                    pyr::BillboardManager::BillboardsRenderData renderData = pyr::BillboardManager::makeContext(selectedBillboards);
                    m_renderBillboardEffect->bindConstantBuffer("CameraBuffer", pcameraBuffer);
                    auto result = renderData.textures
                        | std::views::keys
                        | std::views::transform([](auto texPtr) { return *texPtr; });

                    std::vector<pyr::Texture> textures(result.begin(), result.end());
                    m_renderBillboardEffect->bindTextures(textures, "textures");
                    m_renderBillboardEffect->bind();
                    renderData.instanceBuffer.bind(true);
                    pyr::Engine::d3dcontext().DrawInstanced(6, static_cast<UINT>(renderData.instanceBuffer.getVerticesCount()), 0, 0);
                    m_renderBillboardEffect->unbindResources();
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
            }


     
        };

    }
}
