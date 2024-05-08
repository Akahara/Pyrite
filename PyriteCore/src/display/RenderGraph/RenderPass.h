#pragma once

// Defines an object that takes named entries and named output and draws in a target fbo

#include "NamedResources.h"
#include "display/FrameBuffer.h"
#include "world/Mesh/StaticMesh.h"


namespace pyr
{

    class RenderPass
    {
    protected:

        FrameBuffer m_target;

        std::vector<NamedOutput> m_outputs;
        std::vector<NamedInput> m_inputs;

        std::vector<const StaticMesh*> m_meshes;

    public:

        virtual void apply() = 0;

        //template<class ...Ts>
        //void addNamedOutputs(NamedOutput&& output...) { (m_outputs.push_back(output),...); }
        //template<class ...Ts>
        //void addNamedInput(NamedInput&& input...) { (m_inputs.push_back(input),...); }
        void addMeshToPass(const StaticMesh* meshView) { m_meshes.push_back(meshView); }
    };






}
