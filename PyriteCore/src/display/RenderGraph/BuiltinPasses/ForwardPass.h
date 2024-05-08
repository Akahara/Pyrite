#pragma once

#include "display/RenderGraph/RenderPass.h"
#include "world/Mesh/Mesh.h"
#include "world/Mesh/StaticMesh.h"


namespace pyr
{
    namespace BuiltinPasses
    {

        class ForwardPass : public RenderPass
        {

        private:

            //Camera m_camera;

        public:

            virtual void apply() override
            {

                // Upload MVP :D


                // Render all objects 
                for (const StaticMesh* smesh : m_meshes)
                {
                    smesh->bindModel();
                    smesh->bindMaterial();
                    // todo bind materials and shaders
                    std::span<const SubMesh> subMeshIndices = smesh->getModel()->getRawMeshData()->getSubmeshes();
                    for (int index = 0; index < subMeshIndices.size() - 1; ++index)
                    {
                        const size_t indexCount = subMeshIndices[index + 1].startIndex - subMeshIndices[index].startIndex;
                        Engine::d3dcontext().DrawIndexed(static_cast<UINT>(indexCount), subMeshIndices[index].startIndex, 0);
                    }
                }

            }

        };


    }

}
