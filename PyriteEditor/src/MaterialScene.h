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
        m_ggxShader->setUniform<float>(#name, name);    
 


namespace pye
{

    class MaterialScene : public pyr::Scene
    {
    private:


        pyr::GraphicalResourceRegistry m_registry;
        pyr::Effect* m_ggxShader;

        pyr::Mesh m_ballMesh = pyr::MeshImporter::ImportMeshFromFile(L"res/meshes/boule.obj");
        std::vector<pyr::MaterialMetadata> m_matData = pyr::MeshImporter::FetchMaterialPaths(L"res/meshes/boule.obj");


        pyr::Model m_ballModel;

        std::vector<pyr::StaticMesh> m_balls;
        std::vector<std::shared_ptr<pyr::Material>> m_materials;


        std::shared_ptr<pyr::Material> m_pbrMat;

        using CameraBuffer = pyr::ConstantBuffer < InlineStruct(mat4 mvp; alignas(16) vec3 pos) > ;
        std::shared_ptr<CameraBuffer>           pcameraBuffer = std::make_shared<CameraBuffer>();
        pyr::BuiltinPasses::ForwardPass m_forwardPass;
        pyr::RenderGraph m_RDG;

        pyr::Camera m_camera;
        pyr::FreecamController m_camController;

    public:

        MaterialScene()
        {
            m_ggxShader = m_registry.loadEffect(L"res/shaders/ggx.fx", pyr::InputLayout::MakeLayoutFromVertex<pyr::Mesh::mesh_vertex_t>());

            m_ballModel = pyr::Model{ m_ballMesh };


            m_balls.resize(4, pyr::StaticMesh{ m_ballModel });
            m_materials.resize(4);
            
            m_balls[0].getTransform().position = { -2,0,0 };
            m_balls[1].getTransform().position = { 0,0,0 };
            m_balls[2].getTransform().position = { 2,0,0 };
            m_balls[3].getTransform().position = { 4,0,0 };

            for (int i = 0; i < m_materials.size(); i++)
            {
                m_materials[i] = std::make_shared<pyr::Material>(m_ggxShader);
                m_balls[i].setMaterialOfIndex(0, m_materials[i]);         
                m_forwardPass.addMeshToPass(&m_balls[i]);
            }

            m_RDG.addPass(&m_forwardPass);

            m_camera.setPosition(vec3{0,1,-2});
            m_camera.lookAt(vec3{0,0,0});
            m_camera.setProjection(pyr::PerspectiveProjection{});
            m_camController.setCamera(&m_camera);

            m_ggxShader->addBinding({ .label = "CameraBuffer",   .bufferRef = pcameraBuffer });
        }

        void update(float delta) override
        {
            m_camController.processUserInputs(delta);
            pcameraBuffer->setData(CameraBuffer::data_t{
                .mvp = m_camera.getViewProjectionMatrix(), 
                .pos = m_camera.getPosition()
            });
        }

        void render() override

        {
            ImGui::Begin("MaterialScene");

            static vec3 sunPos = vec3{ 0,-10,0 };
            if (ImGui::SliderFloat3("sunPos", &sunPos.x, -30, 30))
                m_ggxShader->setUniform<vec3>("sunPos", sunPos);

            for (size_t i = 0; i < m_balls.size(); i++)
            {
                ImGui::PushID(i);
                if (ImGui::CollapsingHeader(std::format("Ball #{}", i).c_str()))
                {
                    pyr::StaticMesh& mesh = m_balls[i];
                    auto mat = m_materials[i];
                    pyr::MaterialCoefs coefs = mat->getMaterialCoefs();

                    ImGui::SliderFloat3("Albedo", &coefs.Ka.x, 0 , 1);
                    ImGui::SliderFloat3("Emissive", &coefs.Ke.x, 0 , 1);
                    ImGui::SliderFloat3("Specular", &coefs.Ks.x, 0 , 1);
                    ImGui::SliderFloat("Specular Strength", &coefs.Ns, 0 , 255);
                    ImGui::SliderFloat("IOR", &coefs.Ni, 0 , 1);
                    mat->setMaterialCoefs(coefs);
                }
                ImGui::PopID();
            }


            ImGui::End();

            pyr::RenderProfiles::pushRasterProfile(pyr::RasterizerProfile::NOCULL_RASTERIZER);
            pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTWRITE_DEPTH);
            pyr::Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            m_ggxShader->bindConstantBuffer("CameraBuffer", pcameraBuffer);
            m_forwardPass.getSkyboxEffect()->bindConstantBuffer("CameraBuffer", pcameraBuffer);
            m_RDG.execute();

            pyr::RenderProfiles::popDepthProfile();
            pyr::RenderProfiles::popRasterProfile();
            m_RDG.debugWindow();

        }

        ~MaterialScene() { m_RDG.clearGraph(); }
    };
}