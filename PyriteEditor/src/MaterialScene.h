#pragma once

#include "imgui.h"
#include "display/DebugDraw.h"
#include "display/IndexBuffer.h"
#include "display/InputLayout.h"
#include "display/Vertex.h"
#include "display/VertexBuffer.h"
#include "display/RenderGraph/RenderGraph.h"
#include "display/RenderGraph/BuiltinPasses/BuiltinPasses.h"
#include "engine/Engine.h"
#include "scene/Scene.h"
#include "world/camera.h"
#include "world/Mesh/MeshImporter.h"
#include "display/GraphicalResource.h"
#include "display/RenderProfiles.h"
#include "world/RayCasting.h"

#define IMGUI_DECLARE_FLOAT_UNIFORM(name,shader,a,b) static float name;\
    if (ImGui::SliderFloat(#name, &name, a, b))\
        shader->setUniform<float>(#name, name);    
 


namespace pye
{

    class MaterialScene : public pyr::Scene
    {
    private:


        pyr::GraphicalResourceRegistry m_registry;
        pyr::Effect* m_ggxShader;

        pyr::Mesh m_ballMesh = pyr::MeshImporter::ImportMeshesFromFile(L"res/meshes/boule.obj").at(0); 
        pyr::MaterialMetadata m_pbrMatMetadata{
            .paths = {
                {pyr::TextureType::ALBEDO,      "res/textures/pbr/rock-slab-wall_albedo.dds"},
                {pyr::TextureType::AO,          "res/textures/pbr/rock-slab-wall_ao.dds"},
                {pyr::TextureType::HEIGHT,      "res/textures/pbr/rock-slab-wall_height.dds"},
                {pyr::TextureType::NORMAL,      "res/textures/pbr/rock-slab-wall_normal-dx.dds"},
                {pyr::TextureType::METALNESS,   "res/textures/pbr/rock-slab-wall_metallic.dds"},
                {pyr::TextureType::ROUGHNESS,   "res/textures/pbr/rock-slab-wall_roughness.dds"},
        }
        };


        pyr::Model m_ballModel;

        std::vector<pyr::StaticMesh> m_balls;
        std::vector<std::shared_ptr<pyr::Material>> m_materials;


        using CameraBuffer = pyr::ConstantBuffer < InlineStruct(mat4 mvp; alignas(16) vec3 pos) > ;
        std::shared_ptr<CameraBuffer>           pcameraBuffer = std::make_shared<CameraBuffer>();
        pyr::BuiltinPasses::ForwardPass m_forwardPass;
        pyr::BuiltinPasses::SSAOPass m_SSAOPass;
        pyr::BuiltinPasses::DepthPrePass m_depthPrePass;
        pyr::RenderGraph m_RDG;

        pyr::Camera m_camera;
        pyr::FreecamController m_camController;
        using InverseCameraBuffer = pyr::ConstantBuffer < InlineStruct(mat4 inverseViewProj;  mat4 inverseProj; alignas(16) mat4 Proj) > ;
        std::shared_ptr<InverseCameraBuffer>    pinvCameBuffer = std::make_shared<InverseCameraBuffer>();

    public:

        MaterialScene()
        {
            m_ggxShader = m_registry.loadEffect(L"res/shaders/ggx.fx", pyr::InputLayout::MakeLayoutFromVertex<pyr::Mesh::mesh_vertex_t>());

            m_ballModel = pyr::Model{ &m_ballMesh };

            int gridSize = 7;
            m_balls.resize(gridSize * gridSize, pyr::StaticMesh{ &m_ballModel });
            m_materials.resize(gridSize * gridSize);
            
            for (int i = 0; i < m_materials.size(); i++)
            {
                m_balls[i].getTransform().position = { (i % gridSize) * 2.f , (i / gridSize) * 2.f ,0};
                m_materials[i] = std::make_shared<pyr::Material>(m_ggxShader);
                m_materials[i]->loadMaterialFromMetadata(m_pbrMatMetadata);
                
                m_materials[i]->getMaterialCoefs().Ka = {  1,0,1 };
                m_materials[i]->getMaterialCoefs().Metallic = (i % gridSize) /  (gridSize-1.f );
                m_materials[i]->getMaterialCoefs().Roughness = std::clamp((i / gridSize) / (gridSize-1.f), 0.05f, 1.f);
                m_balls[i].setMaterialOfIndex(0, m_materials[i]);         
                
                m_forwardPass.addMeshToPass(&m_balls[i]);
                m_depthPrePass.addMeshToPass(&m_balls[i]);
            }

            m_RDG.addPass(&m_forwardPass);

            m_camera.setPosition(vec3{  (float)gridSize,(float)gridSize,  - 10});
            m_camera.lookAt(vec3{ (float)gridSize,(float)gridSize,0.f});
            m_camera.setProjection(pyr::PerspectiveProjection{});
            m_camController.setCamera(&m_camera);

            m_ggxShader->addBinding({ .label = "CameraBuffer",   .bufferRef = pcameraBuffer });

            m_RDG.addPass(&m_SSAOPass);
            m_RDG.addPass(&m_forwardPass);
            m_RDG.addPass(&m_depthPrePass);


            m_RDG.getResourcesManager().addProduced(&m_depthPrePass, "depthBuffer");
            m_RDG.getResourcesManager().addProduced(&m_SSAOPass, "ssaoTexture_blurred");
            m_RDG.getResourcesManager().addProduced(&m_SSAOPass, "ssaoTexture");

            m_RDG.getResourcesManager().addRequirement(&m_SSAOPass, "depthBuffer");
            m_RDG.getResourcesManager().linkResource(&m_depthPrePass, "depthBuffer", &m_SSAOPass);

            m_RDG.getResourcesManager().linkResource(&m_SSAOPass, "ssaoTexture_blurred", &m_forwardPass);

            bool bIsGraphValid = m_RDG.getResourcesManager().checkResourcesValidity();

        }

        void update(float delta) override
        {
            m_camController.processUserInputs(delta);
            pcameraBuffer->setData(CameraBuffer::data_t{
                .mvp = m_camera.getViewProjectionMatrix(), 
                .pos = m_camera.getPosition()
            });
            pinvCameBuffer->setData(InverseCameraBuffer::data_t{
                .inverseViewProj = m_camera.getViewProjectionMatrix().Invert(),
                .inverseProj = m_camera.getProjectionMatrix().Invert(),
                .Proj = m_camera.getProjectionMatrix()
                            });
        }

        void render() override

        {
            ImGui::Begin("MaterialScene");
            //static vec3 sunPos = vec3{ 0,-10,0 };
            //if (ImGui::SliderFloat3("sunPos", &sunPos.x, -30, 30))
            //    m_ggxShader->setUniform<vec3>("sunPos", sunPos);

            for (size_t i = 0; i < m_balls.size(); i++)
            {
                ImGui::PushID(i);
                if (ImGui::CollapsingHeader(std::format("Ball #{}", i).c_str()))
                {
                    auto mat = m_materials[i];
                    pyr::MaterialCoefs coefs = mat->getMaterialCoefs();

                    ImGui::SliderFloat3("Albedo", &coefs.Ka.x, 0 , 1);
                    ImGui::SliderFloat3("Emissive", &coefs.Ke.x, 0 , 1);
                    ImGui::SliderFloat3("Specular", &coefs.Ks.x, 0 , 1);
                    ImGui::SliderFloat("Roughness", &coefs.Roughness, 0 , 1);
                    ImGui::SliderFloat("Metallic", &coefs.Metallic, 0 , 1);
                    ImGui::SliderFloat("IOR", &coefs.Ni, 0 , 1);
                    mat->setMaterialCoefs(coefs);
                }
                ImGui::PopID();
            }


            ImGui::End();

            pyr::RenderProfiles::pushRasterProfile(pyr::RasterizerProfile::NOCULL_RASTERIZER);
            pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTWRITE_DEPTH);
            pyr::Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            //m_ggxShader->uploadAllBindings();
            m_ggxShader->bindConstantBuffer("CameraBuffer", pcameraBuffer);
            m_depthPrePass.getDepthPassEffect()->bindConstantBuffer("CameraBuffer", pcameraBuffer);
            m_SSAOPass.getSSAOEffect()->bindConstantBuffer("InverseCameraBuffer", pinvCameBuffer);
            m_SSAOPass.getSSAOEffect()->bindConstantBuffer("CameraBuffer", pcameraBuffer);
            m_forwardPass.getSkyboxEffect()->bindConstantBuffer("CameraBuffer", pcameraBuffer);
            m_RDG.execute();

            pyr::RenderProfiles::popDepthProfile();
            pyr::RenderProfiles::popRasterProfile();
            m_RDG.debugWindow();

        }

        ~MaterialScene() { m_RDG.clearGraph(); }
    };
}