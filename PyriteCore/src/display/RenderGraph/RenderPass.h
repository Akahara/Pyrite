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


        FrameBuffer m_target;

        // this is temp. Maybe we should have a resource manager for the whole graph or somtuhing. idk
        
        std::vector<NamedOutput> m_outputs; // < this is what this pass can provide
        std::unordered_map<std::string, std::function<Texture()>> m_resourceGetters; // this is the getters on name res
        
        std::unordered_map<std::string, NamedInput> m_inputs; // this is what the pass has as inputs
        std::set<std::string> m_requirements; // this is which inputs it needs

        std::vector<const StaticMesh*> m_meshes;


    public:
        bool m_bIsEnabled = true;

        std::string displayName = "N/A";

        void setEnable(bool bEnabled) { m_bIsEnabled = bEnabled; }
        bool isEnabled()  const noexcept { return m_bIsEnabled; }


        virtual void apply() = 0;
        virtual void clear() { m_meshes.clear(); };
        virtual void update(float dt) {};

        bool checkInputsRequirements() const {

            for (const std::string& label : m_requirements)
            {
                // If a requirement is not met (= not in the inputs list
                if (!m_inputs.contains(label)) return false;
            }
            return true;
        }


        void requiresResource(std::string_view resName)
        {
            m_requirements.insert(std::string(resName));
        }

        void producesResource(std::string_view resName, Texture res)
        {
            m_outputs.emplace_back(NamedOutput{ 
                .label = std::string(resName), 
                .res = res, 
                .origin = this,
            });
        }

        void addNamedOutputs(const NamedOutput& output) { m_outputs.push_back(output); }
        void addNamedInput(const NamedInput& input) { m_inputs[input.label] = input; }


        std::optional<NamedOutput> getOutputResource(std::string_view resName) const noexcept
        {
            if (auto it = std::ranges::find_if(m_outputs, [resName](const NamedOutput& r) { return r.label == resName; });
                it != std::end(m_outputs)                    
            ) return *it;
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
