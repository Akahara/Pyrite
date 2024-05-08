#pragma once
#include "RenderPass.h"

namespace pyr
{
    class RenderGraph
    {
    private:

        std::vector<RenderPass*> m_passes;

    public:

        void execute()
        {
            for (auto&& p : m_passes) p->apply();
            // todo blit at the end
        }

        void addPass(RenderPass* pass) { m_passes.emplace_back(pass); }

    };
}
