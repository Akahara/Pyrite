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

#include "display/RenderGraph/RenderPass.h"
#include "display/GraphicalResource.h"
#include "world/Mesh/RawMeshData.h"
#include "world/Mesh/StaticMesh.h"
#include "inputs/UserInputs.h"

#include "editor/EditorActor.h"
#include "editor/bridges/pf_StaticMesh.h"
#include "editor/Editor.h"
#include <world/camera.h>
#include <display/RenderProfiles.h>

namespace pye
{

    namespace EditorPasses
    {

        class WorldHUDPass : public pyr::RenderPass
        {
        private:

            using CameraBuffer = pyr::ConstantBuffer < InlineStruct(mat4 mvp; alignas(16) vec3 pos) > ;
            std::shared_ptr<CameraBuffer>           pcameraBuffer = std::make_shared<CameraBuffer>();

            pyr::GraphicalResourceRegistry m_registry;
            pyr::Effect* m_billboardEffect = nullptr;

        public:


            WorldHUDPass()
            {
                displayName = "Editor-WorldHUD";
                m_billboardEffect = m_registry.loadEffect(
                    L"res/shaders/billboard.fx",
                    pyr::InputLayout::MakeLayoutFromVertex<pyr::EmptyVertex, pyr::Billboard::billboard_vertex_t>()
                );

            }

            virtual void apply() override
            {
                if (!PYR_ENSURE(owner)) return;
                if (!PYR_ENSURE(owner->GetContext().contextCamera)) return;

                auto& Editor = pye::Editor::Get();

                if (Editor.WorldHUD.empty()) return;

                pyr::Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                pyr::RenderProfiles::pushBlendProfile(pyr::BlendProfile::BLEND);

                pcameraBuffer->setData(CameraBuffer::data_t{ .mvp = owner->GetContext().contextCamera->getViewProjectionMatrix(), .pos = owner->GetContext().contextCamera->getPosition() });

                // - First step should be to sort the billboards, whether they are HUD (autofacing, no depth test, depth write for the picker) ect
                std::vector<const pyr::Billboard*> bbs;
                for (auto* editorBB : Editor.WorldHUD)
                {
                    editorBB->editorBillboard->transform.position = editorBB->coreActor->GetTransform().position;
                    bbs.push_back(editorBB->editorBillboard);
                }
                pyr::BillboardManager::BillboardsRenderData renderData = pyr::BillboardManager::makeContext(bbs);
                m_billboardEffect->bindConstantBuffer("CameraBuffer", pcameraBuffer);

                std::vector<pyr::Texture> sortedTextures;
                sortedTextures.resize(16);

                for (const auto& [texPtr, texId] : renderData.textures)
                {
                    sortedTextures[texId] = *texPtr;
                }

                // Collect the view into a vector
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
