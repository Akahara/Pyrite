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

namespace pye
{

    class ForwardPassScene : public pyr::Scene
    {
    private:

        pyr::InputLayout m_layout;
        pyr::Effect* m_baseEffect;

        pyr::RenderGraph m_RDG;

        pyr::Mesh cubeMesh;
        pyr::Model cubeModel;
        pyr::StaticMesh cubeInstance;
        pyr::Material cubeMat;

        pyr::Camera m_camera;
        pyr::FreecamController m_camController;

        pyr::GraphicalResourceRegistry m_grr;
        pyr::Texture m_breadbug;
        pyr::BuiltinPasses::ForwardPass m_forwardPass;

        using CameraBuffer = pyr::ConstantBuffer < InlineStruct(mat4 mvp; alignas(16) vec3 pos) > ;
        using ColorBuffer = pyr::ConstantBuffer < InlineStruct(vec4 colorShift) > ;
        using ImportedBuffer = pyr::ConstantBuffer < InlineStruct(vec4 randomValue) > ;

        std::shared_ptr<CameraBuffer>   pcameraBuffer   = std::make_shared<CameraBuffer>();
        std::shared_ptr<ColorBuffer>    pcolorBuffer    = std::make_shared<ColorBuffer>();
        std::shared_ptr<ImportedBuffer> pimportedBuffer = std::make_shared<ImportedBuffer>();

    public:

        ForwardPassScene()
        {
            drawDebugSetCamera(&m_camera);
            // Import shader and bind cbuffers
            m_layout = pyr::InputLayout::MakeLayoutFromVertex<pyr::Mesh::mesh_vertex_t>();
            m_baseEffect = m_grr.loadEffect(L"res/shaders/mesh.fx", m_layout);
            m_baseEffect->addBinding({ .label = "ColorBuffer",    .bufferRef = pcolorBuffer });
            m_baseEffect->addBinding({ .label = "ImportedBuffer", .bufferRef = pimportedBuffer });
            m_baseEffect->addBinding({ .label = "CameraBuffer",   .bufferRef = pcameraBuffer });

            // Create axes and material (everything is kinda default here)
            cubeMat         = pyr::Material(m_baseEffect);
            cubeMesh        = pyr::MeshImporter::ImportMeshFromFile("res/meshes/axes.obj");
            cubeModel       = pyr::Model{ cubeMesh };
            cubeInstance    = pyr::StaticMesh{ cubeModel, cubeMat };

            // Setup this scene's rendergraph
            m_RDG.addPass(&m_forwardPass);

            m_forwardPass.addMeshToPass(&cubeInstance);

            // Setup the camera
            m_camera.setProjection(pyr::PerspectiveProjection{});
            m_camController.setCamera(&m_camera);

            // Other stuff
            m_breadbug = m_grr.loadTexture(L"res/textures/breadbug.dds");
        }

        void update(double delta) override
        {
            static double elapsed = 0;
            elapsed += delta;
            m_baseEffect->setUniform<double>("u_blue", (sin(elapsed) + 1) / 2.f);
            m_baseEffect->bindTexture(m_breadbug, "tex_breadbug");

            m_camController.processUserInputs(delta);
            
            // Update Cbuffers
            pcolorBuffer->setData(ColorBuffer::data_t{ .colorShift = vec4{0,0,1,1}});
            pimportedBuffer->setData(ImportedBuffer::data_t{ .randomValue = vec4{1,0,0,0} });
            pcameraBuffer->setData(CameraBuffer::data_t{ .mvp = m_camera.getViewProjectionMatrix(), .pos = m_camera.getPosition()});
        }
        
        void render() override

        {
          
            ImGui::Begin("cam");
            if (vec3 euler = m_camera.getRotation().ToEuler(); ImGui::DragFloat3("rot", &euler.x, .1f))
              m_camera.setRotation(quat::CreateFromYawPitchRoll(euler));
            if (vec3 p = m_camera.getPosition(); ImGui::DragFloat3("pos", &p.x))
              m_camera.setPosition(p);
            ImGui::End();

            pyr::RenderProfiles::pushRasterProfile(pyr::RasterizerProfile::NOCULL_RASTERIZER);
            pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTWRITE_DEPTH);
            pyr::Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            m_baseEffect->uploadAllBindings();
            m_forwardPass.getSkyboxEffect()->bindConstantBuffer("CameraBuffer", pcameraBuffer);
            m_RDG.execute();

            pyr::drawDebugLine(vec3::Zero, vec3::UnitX * 10.f, { 1,0,0,1 });
            pyr::drawDebugLine(vec3::Zero, vec3::UnitY * 10.f, { 0,1,0,1 });
            pyr::drawDebugLine(vec3::Zero, vec3::UnitZ * 10.f, { 0,0,1,1 });
            pyr::drawDebugBox(vec3::Zero, vec3(5.f));

            pyr::RenderProfiles::popDepthProfile();
            pyr::RenderProfiles::popRasterProfile();

        }

        ~ForwardPassScene() override
        {
            m_RDG.clearGraph();
        }
    };
}
