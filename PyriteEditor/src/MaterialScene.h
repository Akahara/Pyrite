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
        pyr::Effect* m_equiproj;

        std::shared_ptr<pyr::Model> m_ballModel = pyr::MeshImporter::ImportMeshesFromFile(L"res/meshes/boule.obj").at(0);
        std::shared_ptr<pyr::Model> m_cubeModel = pyr::MeshImporter::ImportMeshesFromFile(L"res/meshes/cube.obj").at(0);
        std::vector<pyr::StaticMesh> m_balls;

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
        pyr::Texture m_hdrMap;

    public:

        MaterialScene()
        {
            m_hdrMap = m_registry.loadTexture(L"textures/HDR/2.hdr");
            m_equiproj = m_registry.loadEffect(L"res/shaders/EquirectangularProjection.fx", pyr::InputLayout::MakeLayoutFromVertex<pyr::RawMeshData::mesh_vertex_t>());
            m_ggxShader = m_registry.loadEffect(L"res/shaders/ggx.fx", pyr::InputLayout::MakeLayoutFromVertex<pyr::RawMeshData::mesh_vertex_t>());

#pragma region BALLS
            int gridSize = 7;
            m_balls.resize(gridSize * gridSize, pyr::StaticMesh{ m_ballModel });
            for (int i = 0; i < m_balls.size(); i++)
            {
                m_balls[i].getTransform().position = { (i % gridSize) * 2.f , (i / gridSize) * 2.f ,0};
                
                pyr::MaterialRenderingCoefficients coefs;
                coefs.Ka = { 1,0,1 };
                coefs.Metallic = (i % gridSize) / (gridSize - 1.f);
                coefs.Roughness = std::clamp((i / gridSize) / (gridSize - 1.f), 0.05f, 1.f);
                auto mat = pyr::Material::MakeRegisteredMaterial({}, coefs, m_ggxShader, std::format("Material_%d",i));
                m_balls[i].overrideSubmeshMaterial(0, mat);
                //m_forwardPass.addMeshToPass(&m_balls[i]);
                //m_depthPrePass.addMeshToPass(&m_balls[i]);
            }
#pragma endregion BALLS

#pragma region RDG
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
            m_forwardPass.boundCamera = &m_camera;
            bool bIsGraphValid = m_RDG.getResourcesManager().checkResourcesValidity();
#pragma endregion RDG
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
            static vec3 sunPos = vec3{ 0,100,0 };
            if (ImGui::SliderFloat3("sunPos", &sunPos.x, -300, 300))
                m_ggxShader->setUniform<vec3>("sunPos", sunPos);

            ImGui::End();

            pyr::Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            
            
            pyr::RenderProfiles::pushRasterProfile(pyr::RasterizerProfile::NOCULL_RASTERIZER);

            m_equiproj->bindConstantBuffer("CameraBuffer", pcameraBuffer);
            m_equiproj->bind();
            m_cubeModel->bind();
            m_equiproj->bindTexture(m_hdrMap, "mat_hdr");
            pyr::Engine::d3dcontext().DrawIndexed(36,0, 0);
            m_equiproj->unbindResources();


            pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTWRITE_DEPTH);
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