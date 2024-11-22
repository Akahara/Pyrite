#pragma once
#include "RenderPass.h"
#include "scene/RenderableActorCollection.h"
#include "RDGResourcesManager.h"

static inline PYR_DEFINELOG(LogRenderGraph, VERBOSE);

namespace pyr
{
    // This is given through the execute and can be used and accessed by any passes.
    struct RenderContext
    {
        RegisteredRenderableActorCollection ActorsToRender; // make this a ref
        std::string debugName = "Main scene render graph"; 
        pyr::Camera* contextCamera = nullptr; 
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
        const RenderGraphResourceManager& getResourcesManager() const noexcept { return m_manager; }
        const RenderContext& GetContext() const { return m_renderContext; }
        RenderContext& GetContext() { return m_renderContext; }

    public:

        void execute(const RenderContext& frameRenderContext = {});
        void addPass(RenderPass* pass)  { m_passes.emplace_back(pass); m_manager.addNewPass(pass); pass->owner = this; }
        void debugWindow() {}

    };
}
