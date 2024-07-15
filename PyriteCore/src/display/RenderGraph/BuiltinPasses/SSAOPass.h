#pragma once

#include <imgui.h>

#include "display/RenderGraph/RenderPass.h"
#include "display/GraphicalResource.h"
#include "world/Mesh/RawMeshData.h"
#include "world/Mesh/StaticMesh.h"
#include "utils/math.h"

namespace pyr
{

    namespace BuiltinPasses
    {
        class SSAOPass : public RenderPass
        {
        private:

            pyr::GraphicalResourceRegistry m_registry;

            FrameBuffer m_ssaoTextureTarget{ Device::getWinWidth(), Device::getWinHeight(), FrameBuffer::COLOR_0 };
            FrameBuffer m_blurredSSAOTarget{ Device::getWinWidth(), Device::getWinHeight(), FrameBuffer::COLOR_0 };

            Effect* m_ssaoEffect = nullptr;
            Effect* m_blurEffect = nullptr;

            std::vector<vec4> m_kernel;
            Texture m_randomTexture;

        public:


            Effect* getSSAOEffect() const noexcept { return m_ssaoEffect; }

            SSAOPass()
            {
                displayName = "SSAO (Pass + Blur)"; 

                m_ssaoEffect = m_registry.loadEffect(
                    L"res/shaders/computeSSAO.fx",
                    InputLayout::MakeLayoutFromVertex<EmptyVertex>()
                );

                m_blurEffect = m_registry.loadEffect(
                    L"res/shaders/gaussianBlur.fx",
                    InputLayout::MakeLayoutFromVertex<EmptyVertex>()
                );


                m_randomTexture = m_registry.loadTexture(L"res/textures/randomNoise.dds");
                m_kernel = generateKernel(64);

                producesResource("ssaoTexture", m_ssaoTextureTarget.getTargetAsTexture(FrameBuffer::COLOR_0));
                producesResource("ssaoTexture_blurred", m_blurredSSAOTarget.getTargetAsTexture(FrameBuffer::COLOR_0));
            }

            virtual void apply() override
            {
                // Compute the SSAO Texture
                m_ssaoTextureTarget.clearTargets();
                m_ssaoTextureTarget.bind();

                m_ssaoEffect->bindTexture(m_inputs["depthBuffer"].res, "depthBuffer");
                m_ssaoEffect->bindTexture(m_randomTexture, "blueNoise");
                m_ssaoEffect->setUniform<std::vector<vec4>>("u_kernel", m_kernel);
                m_ssaoEffect->bind();
                
                Engine::d3dcontext().DrawIndexed(3, 0, 0);

                m_ssaoEffect->unbindResources();

                // Blur pass
                m_ssaoTextureTarget.unbind();
                m_blurredSSAOTarget.bind();
                m_blurEffect->bindTexture(m_ssaoTextureTarget.getTargetAsTexture(FrameBuffer::COLOR_0), "sourceTexture");
                m_blurEffect->bind();
                Engine::d3dcontext().DrawIndexed(3, 0, 0);
                m_blurEffect->unbindResources();
                m_blurredSSAOTarget.unbind();

                debugWindow();
            }

            void debugWindow()
            {
                ImGui::Begin("SSAO Pass Debug");

                ImGui::Image((void*)m_inputs["depthBuffer"].res.getRawTexture(), ImVec2{ 256,256 });
                ImGui::Image((void*)m_ssaoTextureTarget.getTargetAsTexture(FrameBuffer::COLOR_0).getRawTexture(), ImVec2{256,256});
                ImGui::Image((void*)m_blurredSSAOTarget.getTargetAsTexture(FrameBuffer::COLOR_0).getRawTexture(), ImVec2{256,256});

                static float sampleRad = 1.5f;
                static float u_bias = 0.0001f;
                static float u_noiseSclae = 10.f;
                static float u_tolerancy = -0.85f;
                static float u_alpha = 0.005f;
                static float u_blurStrength = 1.f;
                
                if (ImGui::SliderFloat("sampleRad", &sampleRad, 0, 5))              m_ssaoEffect->setUniform<float>("u_sampleRad",sampleRad);
                if (ImGui::SliderFloat("u_bias", &u_bias, -0.001f, 0.001f))           m_ssaoEffect->setUniform<float>("u_bias", u_bias);
                if (ImGui::SliderFloat("u_noiseScale", &u_noiseSclae, 0, 10))       m_ssaoEffect->setUniform<float>("u_noiseScale", u_noiseSclae);
                if (ImGui::SliderFloat("u_tolerancy", &u_tolerancy, -1, 1))         m_ssaoEffect->setUniform<float>("u_tolerancy", u_tolerancy);
                if (ImGui::SliderFloat("u_alpha", &u_alpha, 0.f, 0.25f))               m_ssaoEffect->setUniform<float>("u_alpha", u_alpha);
                if (ImGui::SliderFloat("u_blurStrength", &u_blurStrength, 0, 10))   m_blurEffect->setUniform<float>("u_blurStrength", u_blurStrength);

                ImGui::End();


            }

        private:

            std::vector<vec4> generateKernel(int sampleCount = 16)
            {
                std::vector<vec4> kernel;
                kernel.resize(sampleCount);
                auto rng = mathf::randomFunction();

                for (size_t sampleId = 0; sampleId < sampleCount; sampleId++)
                {
                    vec4 sample = vec4{
                        rng() * 2.F - 1.F,
                        rng() * 2.F - 1.F,
                        rng() ,
                        0
                    };
                    sample.Normalize();
                    kernel[sampleId] = sample * static_cast<float>(sampleId) / static_cast<float>(sampleCount);
                }
                return kernel;
            }

        };
    }
}
