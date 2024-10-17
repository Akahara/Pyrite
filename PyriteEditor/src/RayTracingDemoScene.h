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

namespace pye {

class RayTracingDemoScene : public pyr::Scene {
private:

  pyr::InputLayout m_layout;
  pyr::Effect *m_baseEffect;

  pyr::RenderGraph m_RDG;
  pyr::BuiltinPasses::ForwardPass m_forwardPass;

  std::shared_ptr<pyr::Model> cubeModel;
  pyr::StaticMesh cubeInstance;

  pyr::Camera m_camera;
  pyr::FreecamController m_camController;

  pyr::GraphicalResourceRegistry m_grr;

  using CameraBuffer = pyr::ConstantBuffer<InlineStruct(mat4 mvp; alignas(sizeof vec4) vec3 pos)>;
  using ColorBuffer = pyr::ConstantBuffer<InlineStruct(vec4 colorShift)>;
  using ImportedBuffer = pyr::ConstantBuffer<InlineStruct(vec4 randomValue)>;

  std::shared_ptr<CameraBuffer> pcameraBuffer = std::make_shared<CameraBuffer>();

public:

  RayTracingDemoScene()
  {
    // Import shader and bind cbuffers
    m_layout = pyr::InputLayout::MakeLayoutFromVertex<pyr::RawMeshData::mesh_vertex_t>();
    m_baseEffect = m_grr.loadEffect(L"res/shaders/mesh.fx", m_layout);
    m_baseEffect->addBinding({ .label = "CameraBuffer", .bufferRef = pcameraBuffer });

    // Create axes and material (everything is kinda default here)
    cubeModel = pyr::MeshImporter::ImportMeshesFromFile("res/meshes/axes.obj").at(0);
    cubeInstance = pyr::StaticMesh{ cubeModel };
    //cubeInstance.setBaseMaterial(std::make_shared<pyr::Material>(m_baseEffect));
    Transform& cubeTransform = cubeInstance.GetTransform();
    cubeTransform = Transform{ vec3(1,2,3), vec3(1,.5f,2.f), quat::CreateFromAxisAngle(mathf::normalize(vec3(1,2,3)), 1.f) };

    // Setup this scene's rendergraph
    m_RDG.addPass(&m_forwardPass);
    m_RDG.getResourcesManager().checkResourcesValidity();
    
    // Setup the camera
    m_camera.setProjection(pyr::PerspectiveProjection{});
    m_camController.setCamera(&m_camera);
    drawDebugSetCamera(&m_camera);
    m_forwardPass.boundCamera = &m_camera;
  }

  void update(float delta) override
  {
    static double elapsed = 0;
    elapsed += delta;

    m_camController.processUserInputs(delta);
  }

  void render() override
  {
    SceneActors.registerForFrame(&cubeInstance);
    ImGui::Begin("raytrace");
    static vec3 p0{ 3,3,3 }, p1{ -2,-3,-4 };
    ImGui::DragFloat3("P0", &p0.x, .25f);
    ImGui::DragFloat3("P1", &p1.x, .25f);
    pyr::RayResult hit = pyr::raytrace(cubeInstance, { p0, mathf::normalize(p1 - p0) });
    pyr::drawDebugLine(p0, p1, vec4{ 1,0,0,1 });
    pyr::drawDebugSphere(p0, .1f, vec4{ 1,.5f,0,1 });
    pyr::drawDebugSphere(p1, .1f, vec4{ 1,0,.5f,1 });
    ImGui::BeginDisabled();
    ImGui::Checkbox("hit", &hit.bHit);
    ImGui::DragFloat3("position", &hit.position.x);
    ImGui::DragFloat3("normal", &hit.normal.x);
    ImGui::EndDisabled();
    ImGui::End();
    if (hit.bHit) {
      pyr::drawDebugSphere(hit.position, .5f);
      pyr::drawDebugLine(hit.position, hit.position + hit.normal);
    }

    pyr::RenderProfiles::pushRasterProfile(pyr::RasterizerProfile::CULLBACK_RASTERIZER);
    pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTWRITE_DEPTH);
    pyr::Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    pcameraBuffer->setData(CameraBuffer::data_t{ .mvp = m_camera.getViewProjectionMatrix(), .pos = m_camera.getPosition() });
    m_baseEffect->uploadAllBindings();
    m_forwardPass.getSkyboxEffect()->bindConstantBuffer("CameraBuffer", pcameraBuffer);
    m_RDG.execute(pyr::RenderContext{ SceneActors });

    pyr::RenderProfiles::popDepthProfile();
    pyr::RenderProfiles::popRasterProfile();
  }
};
}
