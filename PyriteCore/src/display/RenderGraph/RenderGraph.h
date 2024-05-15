#pragma once
#include "RenderPass.h"

namespace pyr
{

    struct InvalidGraph {};

    class RenderGraph
    {
    private:

        std::vector<RenderPass*> m_passes;

    public:




        void execute()
        {
            for (RenderPass* p : m_passes)
            {
                if (p->isEnabled()) p->apply();
            }
        }

        void addPass(RenderPass* pass) { m_passes.emplace_back(pass); }
        void clearGraph()
        {
            for (RenderPass* p : m_passes) p->clear();
        }

    public:

        void linkResource(RenderPass* from, std::string_view resName, RenderPass* to)
        {
            auto Output = from->getOutputResource(resName);
            if (Output.has_value())
                to->addNamedInput(Output.value());
            else throw 3; // resource output not found
        }

        void ensureGraphValidity()
        {
            for (RenderPass* p : m_passes)
                if (!p->checkInputsRequirements()) throw InvalidGraph{};
        }

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
