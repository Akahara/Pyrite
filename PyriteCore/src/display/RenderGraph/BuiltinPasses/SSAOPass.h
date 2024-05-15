#pragma once

#include "display/RenderGraph/RenderPass.h"
#include "display/GraphicalResource.h"
#include "world/Mesh/Mesh.h"
#include "world/Mesh/StaticMesh.h"
#include "utils/math.h"
#include <imgui.h>


namespace pyr
{

    namespace BuiltinPasses
    {

        class SSAOPass : public RenderPass
        {
        private:

            pyr::GraphicalResourceRegistry m_registry;

            FrameBuffer m_ssaoTextureTarget{ Device::getWinWidth(), Device::getWinHeight(), FrameBuffer::COLOR_0 };

            Effect* m_ssaoEffect = nullptr;
            std::vector<vec4> m_kernel;
            Texture m_blueNoise;

        private:

            std::vector<vec4> generateKernel(int sampleCount = 16)
            {
                std::vector<vec4> kernel;
                kernel.resize(sampleCount);
                auto rng = mathf::randomFunction();
                
                for (int sampleId = 0; sampleId < sampleCount; sampleId++)
                {
                    vec4 sample = vec4{
                        rng() * 2.F - 1.F,
                        rng() * 2.F - 1.F,
                        rng() ,
                        0
                    };
                    sample.Normalize();
                    kernel[sampleId] = sample * sampleId / sampleCount;
                }

                return kernel;

            }

        public:


            Effect* getSSAOEffect() const noexcept { return m_ssaoEffect; }

            SSAOPass()
            {

                requiresResource("depthBuffer");

                m_ssaoEffect = m_registry.loadEffect(
                    L"res/shaders/computeSSAO.fx",
                    InputLayout::MakeLayoutFromVertex<EmptyVertex>()
                );

                m_kernel = generateKernel(64);

                m_blueNoise = m_registry.loadTexture(L"res/textures/randomNoise.dds");
                m_ssaoEffect->setUniform<float>("u_sampleRad", 0.5f);
                producesResource("ssaoTexture", m_ssaoTextureTarget.getTargetAsTexture(FrameBuffer::COLOR_0));
            }

            virtual void apply() override
            {
                // Render all objects to a depth only texture


                m_ssaoTextureTarget.clearTargets();
                m_ssaoTextureTarget.bind();

                m_ssaoEffect->bind();
                m_ssaoEffect->bindTexture(m_inputs["depthBuffer"].res, "depthBuffer");
                m_ssaoEffect->bindTexture(m_blueNoise, "blueNoise");
                m_ssaoEffect->setUniform<std::vector<vec4>>("u_kernel", m_kernel);
                
                Engine::d3dcontext().DrawIndexed(3, 0, 0);

                m_ssaoEffect->unbindResources();

                m_ssaoTextureTarget.unbind();
                debugWindow();
            }

            void debugWindow()
            {
                ImGui::Begin("SSAO Pass Debug");

                ImGui::Image((void*)m_inputs["depthBuffer"].res.getRawTexture(), ImVec2{ 256,256 });
                ImGui::Image((void*)m_ssaoTextureTarget.getTargetAsTexture(FrameBuffer::COLOR_0).getRawTexture(), ImVec2{256,256});

                static float sampleRad = 1.5f;
                if (ImGui::SliderFloat("sampleRad", &sampleRad, 0, 5))
                {
                    m_ssaoEffect->setUniform<float>("u_sampleRad",sampleRad);
                }

                static float u_bias = 0.0001f;
                if (ImGui::SliderFloat("u_bias", &u_bias, -0.001, 0.001))
                {
                    m_ssaoEffect->setUniform<float>("u_bias", u_bias);
                }

                static float u_noiseSclae = 10;
                if (ImGui::SliderFloat("u_noiseScale", &u_noiseSclae, 0, 10))
                {
                    m_ssaoEffect->setUniform<float>("u_noiseScale", u_noiseSclae);
                }

                static float u_tolerancy = -0.85f;
                if (ImGui::SliderFloat("u_tolerancy", &u_tolerancy, -1, 1))
                {
                    m_ssaoEffect->setUniform<float>("u_tolerancy", u_tolerancy);
                }

                static float u_alpha = 0.005f;
                if (ImGui::SliderFloat("u_alpha", &u_alpha, 0, 0.25))
                {
                    m_ssaoEffect->setUniform<float>("u_alpha", u_alpha);
                }


                ImGui::End();


            }

        };
    }
}
