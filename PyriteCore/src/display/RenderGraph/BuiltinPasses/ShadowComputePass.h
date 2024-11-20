#pragma once

#include "display/RenderGraph/RenderPass.h"
#include "display/RenderGraph/RenderGraph.h"
#include "display/GraphicalResource.h"
#include "world/Mesh/RawMeshData.h"
#include "world/Mesh/StaticMesh.h"
#include "world/Tools/CommonConstantBuffers.h"
#include "display/FrameBuffer.h"

namespace pyr
{

    namespace BuiltinPasses
    {

        class ShadowComputePass : public RenderPass
        {
        private:

            pyr::GraphicalResourceRegistry m_registry;


        public:

            ShadowComputePass(unsigned int width, unsigned int height) {}

            virtual void apply() override
            {
            }


        };
    }
}
