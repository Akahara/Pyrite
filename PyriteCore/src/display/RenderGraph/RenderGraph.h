#pragma once
#include "RenderPass.h"
#include "scene/RenderableActorCollection.h"
#include "RDGResourcesManager.h"

namespace pyr
{
    // This is given through the execute and can be used and accessed by any passes.
    struct RenderContext
    {
        RegisteredRenderableActorCollection ActorsToRender;
    };


    class RenderGraph
    {
    private:

        std::vector<RenderPass*> m_passes;
        RenderGraphResourceManager m_manager;

        // -- Should be valid for a frame, contains what the camera is supposed to see (for now, since we dont have frustum culling, this context should be equal to the scene actors)
        RenderContext m_renderContext;

    public:

        RenderGraphResourceManager& getResourcesManager() noexcept { return m_manager; }

        const RenderContext& GetContext() const { return m_renderContext; }
        void execute(const RenderContext& frameRenderContext = {}) { m_renderContext = frameRenderContext ; for (RenderPass* p : m_passes) if (p->isEnabled()) p->apply(); }
        void addPass(RenderPass* pass)  { m_passes.emplace_back(pass); m_manager.addNewPass(pass); pass->owner = this; }
        void debugWindow()
        {
            // Shitty stuff, will rewrite this to a proper graph display usin imguizmo
            //ImGui::Begin("RenderGraph");
            //for (auto& pass : m_passes )
            //{
            //    ImGui::Checkbox(pass->displayName.c_str(), &pass->m_bIsEnabled);
            //}
            //ImGui::End();
        }

    };
}
