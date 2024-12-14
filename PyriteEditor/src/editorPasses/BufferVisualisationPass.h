#pragma once

#include "display/RenderGraph/RenderPass.h"
#include "display/RenderGraph/RenderGraph.h"
#include "display/GraphicalResource.h"
#include "world/Mesh/RawMeshData.h"
#include "world/Mesh/StaticMesh.h"
#include "world/Tools/CommonConstantBuffers.h"
#include "display/FrameBuffer.h"

#include "inputs/UserInputs.h"

#include <variant>
#include <optional>

namespace pye
{

    namespace EditorPasses
    {

        class BufferVisualisationPass : public pyr::RenderPass
        {
        private:

            pyr::GraphicalResourceRegistry m_registry;

            pyr::Effect* m_blitEffect = nullptr;
            std::optional<pyr::NamedResource::resource_t> m_resource;

            enum { NO_TEXTURE_ARRAY_INDEX = -1};
            int requestedLayerVisualisation = 0;

        public:

            BufferVisualisationPass()
            {
                displayName = "(DEBUG) Editor - Buffer visualisation";
                m_blitEffect = m_registry.loadEffect(
                    L"res/shaders/debug_blit.fx",
                    pyr::InputLayout::MakeLayoutFromVertex<pyr::EmptyVertex>()
                );



            }

            virtual void apply() override
            {
                if (!PYR_ENSURE(owner)) return;
                static pyr::RenderGraph* cachedRenderGraph = nullptr;
                if (cachedRenderGraph != owner)
                {
                    cachedRenderGraph = owner;
                    RegisterSceneResourcesIntoSubmenus();
                }

                if (!m_resource.has_value()) return;

                if (std::holds_alternative<pyr::Texture>( m_resource.value() ))
                {
                    pyr::Texture texture = std::get<pyr::Texture>(*m_resource);
                    m_blitEffect->bindTexture(texture, "VisualisedBuffer");
                    m_blitEffect->setUniform<int>("u_BufferArrayVisualisedIndex", NO_TEXTURE_ARRAY_INDEX);
                }

                else if (std::holds_alternative<pyr::TextureArray>(m_resource.value()))
                {
                    pyr::TextureArray texturearray = std::get<pyr::TextureArray>(*m_resource);
                    if (pyr::UserInputs::consumeKeyPress(keys::SC_UP_ARROW))
                    {
                        requestedLayerVisualisation = std::min<int>(requestedLayerVisualisation + 1, (int)texturearray.getTextureOrCubeCount());
                    }

                    if (pyr::UserInputs::consumeKeyPress(keys::SC_DOWN_ARROW))
                    {
                        requestedLayerVisualisation = std::max<int>(requestedLayerVisualisation - 1, 0);
                    }

                    m_blitEffect->bindTexture(texturearray, "BufferArray");
                    m_blitEffect->setUniform<int>("u_BufferArrayVisualisedIndex", requestedLayerVisualisation);

                } 
                else {
                    PYR_ASSERT(false, "Cubemap visualisation is not supported yet ! Too bad !");
                }


                m_blitEffect->bind();
                pyr::Engine::d3dcontext().Draw(3, 0);
                m_blitEffect->unbindResources();

            }

            void StartVisualising(const pyr::NamedResource::resource_t& resource)
            {
                m_resource = resource;
            }

            void StopVisualisation() { m_resource = std::nullopt; }

            void RegisterSceneResourcesIntoSubmenus()
            {
                pye::widgets::EditorUI::ImGui_MainBar_MenuEntry  submenu;
                submenu.item_name = "Buffer visualisation";

                pye::widgets::EditorUI::ImGui_MainBar_MenuEntry  reset;
                reset.item_name = "No-Visualisation";
                reset.OnItemClicked.BindCallback([this]() { StopVisualisation(); });
                submenu.children.push_back(std::move(reset));

                for (const auto& [pass, resource] : owner->getResourcesManager().GetAllResources())
                {
                    for (const auto& [name, ref] : resource.producedResources)
                    {
                        pye::widgets::EditorUI::ImGui_MainBar_MenuEntry  submenu_entry;
                        submenu_entry.item_name = name;
                        submenu_entry.OnItemClicked.BindCallback([&ref, this]() { StartVisualising(ref.res); });
                        submenu.children.push_back(std::move(submenu_entry));
                    }
                }

                pye::widgets::EditorUI::Get().GetMenu("Tools").push_back(std::move(submenu));
            }

        };
    }
}
