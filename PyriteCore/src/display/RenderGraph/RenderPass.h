#pragma once

// Defines an object that takes named entries and named output and draws in a target fbo

#include "NamedResources.h"
#include "display/FrameBuffer.h"
#include "world/Mesh/StaticMesh.h"
#include <set>


namespace pyr
{

    class RenderPass
    {
    protected:


        // this is temp. Maybe we should have a resource manager for the whole graph or somtuhing. idk

        std::vector<const StaticMesh*> m_meshes;

        std::unordered_map<std::string, NamedInput> m_inputs; // this is what the pass has as inputs
        std::unordered_map<std::string, ResourceGetter> m_outputs;

        std::set<std::string> m_requirements;

    public:

        bool m_bIsEnabled = true;
        std::string displayName = "N/A";

        void setEnable(bool bEnabled) { m_bIsEnabled = bEnabled; }
        bool isEnabled()  const noexcept { return m_bIsEnabled; }


        virtual void apply() = 0;
        virtual void clear() { m_meshes.clear(); };
        virtual void update(float dt) {}; // should be useless, idk yet


        void producesResource(const char* resName, Texture textureHandle) {
            m_outputs[resName] = [textureHandle]() -> Texture { return textureHandle; };
        }

        void addNamedInput(const NamedInput& input) { m_inputs[input.label] = input; }

        // This is bad
        std::optional<NamedOutput> getOutputResource(const char* resName)  noexcept
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

        void addMeshToPass(const StaticMesh* meshView) { m_meshes.push_back(meshView); }

    };






}
