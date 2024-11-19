#pragma once

// Defines an object that takes named entries and named output and draws in a target fbo

#include "NamedResources.h"
#include "utils/Debug.h"
#include <set>
#include <optional>

static inline PYR_DEFINELOG(LogRenderPass, VERBOSE);

namespace pyr
{
    class RenderPass
    {
    protected:


        // this is temp. Maybe we should have a resource manager for the whole graph or somtuhing. idk
        std::unordered_map<std::string, NamedInput> m_inputs; // this is what the pass has as inputs
        std::unordered_map<std::string, ResourceGetter> m_outputs;

        std::set<std::string> m_requirements;

    public:

        virtual ~RenderPass() = default;

        class RenderGraph* owner;
        bool m_bIsEnabled = true;
        std::string displayName = "N/A";

        void setEnable(bool bEnabled) { m_bIsEnabled = bEnabled; }
        bool isEnabled()  const noexcept { return m_bIsEnabled; }

        virtual void OpenDebugWindow() { }
        virtual bool HasDebugWindow() { return false;  }


        virtual void apply() = 0;
        virtual void update(float dt) {}; // should be useless, idk yet


        void producesResource(const char* resName, Texture textureHandle) {
            m_outputs[resName] = [textureHandle]() -> Texture { return textureHandle; };
        }

        void addNamedInput(const NamedInput& input) { m_inputs[input.label] = input; }

        // This is bad
        std::optional<NamedOutput> getOutputResource(const char* resName) noexcept
        {
            if (m_outputs.contains(resName)) return NamedOutput 
            {   
                .label = resName, 
                .res = m_outputs[resName](),
                .origin = this, 
            };

            return std::nullopt;
        }
        std::optional<NamedInput> getInputResource(std::string_view resName) const noexcept
        {
            if (m_inputs.contains(std::string(resName)))
            {
                const NamedInput& namedInput = m_inputs.at(std::string(resName));
                if (!namedInput.origin->m_bIsEnabled) return std::nullopt;
                return namedInput;
            }

            return std::nullopt;
        }

    };






}
