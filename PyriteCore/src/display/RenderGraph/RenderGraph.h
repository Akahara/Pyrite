#pragma once
#include "RenderPass.h"
#include "RDGResourcesManager.h"

namespace pyr
{
    class RenderGraph
    {
    private:

        std::vector<RenderPass*> m_passes;
        RenderGraphResourceManager m_manager;

    public:

        RenderGraphResourceManager& getResourcesManager() noexcept { return m_manager; }

        void execute()                  { for (RenderPass* p : m_passes) if (p->isEnabled()) p->apply(); }
        void clearGraph()               { for (RenderPass* p : m_passes) p->clear(); }
        void addPass(RenderPass* pass)  { m_passes.emplace_back(pass); m_manager.addNewPass(pass); }

        void debugWindow()
        {
            ImGui::Begin("RenderGraph");
            for (auto& pass : m_passes )
            {
                ImGui::Checkbox(pass->displayName.c_str(), &pass->m_bIsEnabled);
            }
            ImGui::End();
        }

    };
}
