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

class VoxelisationDemoScene : public pyr::Scene {
private:

  pyr::InputLayout m_layout;
  pyr::Effect *m_baseEffect;

  pyr::Camera m_camera;
  pyr::FreecamController m_camController;

  pyr::GraphicalResourceRegistry m_grr;

  using CameraBuffer = pyr::ConstantBuffer<InlineStruct(mat4 mvp; alignas(sizeof vec4) vec3 pos)>;

  std::shared_ptr<CameraBuffer> pcameraBuffer = std::make_shared<CameraBuffer>();

  using CubeVertex = pyr::GenericVertex<pyr::POSITION>;
  using CubeInstance = pyr::GenericVertex<pyr::INSTANCE_TRANSFORM, pyr::INSTANCE_COLOR>;
  pyr::VertexBuffer m_cubeVB;
  pyr::VertexBuffer m_cubeInstanceBuffer;
  pyr::IndexBuffer m_cubeIB;

public:

  VoxelisationDemoScene()
  {
    m_layout = pyr::InputLayout::MakeLayoutFromVertex<CubeVertex, CubeInstance>();
    m_baseEffect = m_grr.loadEffect(L"res/shaders/debug_cubes.fx", m_layout);
    m_baseEffect->addBinding({ .label = "CameraBuffer", .bufferRef = pcameraBuffer });

    std::vector<CubeVertex> cubeVertices{ {
      { vec4(-1, -1, -1, 1) },
      { vec4(+1, -1, -1, 1) },
      { vec4(+1, +1, -1, 1) },
      { vec4(-1, +1, -1, 1) },
      { vec4(-1, -1, +1, 1) },
      { vec4(+1, -1, +1, 1) },
      { vec4(+1, +1, +1, 1) },
      { vec4(-1, +1, +1, 1) },
    } };
    std::vector<pyr::IndexBuffer::size_type> cubeIndices{ {
      0, 1, 3, 3, 1, 2,
      1, 5, 2, 2, 5, 6,
      5, 4, 6, 6, 4, 7,
      4, 0, 7, 7, 0, 3,
      3, 2, 7, 7, 2, 6,
      4, 5, 0, 0, 5, 1,
    } };
    std::vector<CubeInstance> cubeInstances{ {
      { Transform{ vec3(0.f), vec3(1.f), quat::Identity }.getWorldMatrix(), vec4(1,0,1,1) },
      { Transform{ vec3(0,1,0), vec3(.5f), quat::Identity }.getWorldMatrix(), vec4(1,1,1,1) },
      { Transform{ vec3(2), vec3(2.f), quat::CreateFromAxisAngle(vec3::Up, PI*.25f) }.getWorldMatrix(), vec4(1,0,0,.25f) }
    } };
    m_cubeIB = pyr::IndexBuffer(cubeIndices);
    m_cubeVB = pyr::VertexBuffer(cubeVertices);
    m_cubeInstanceBuffer = pyr::VertexBuffer(cubeInstances, true);

    m_camera.setProjection(pyr::PerspectiveProjection{});
    m_camController.setCamera(&m_camera);
    drawDebugSetCamera(&m_camera);
  }

  void update(float delta) override
  {
    m_camController.processUserInputs(delta);
  }

  void render() override
  {
    pyr::RenderProfiles::pushRasterProfile(pyr::RasterizerProfile::NOCULL_RASTERIZER);
    pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTWRITE_DEPTH);
    pyr::Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    pcameraBuffer->setData(CameraBuffer::data_t{ .mvp = m_camera.getViewProjectionMatrix(), .pos = m_camera.getPosition() });
    m_baseEffect->uploadAllBindings();
    m_baseEffect->bind();
    m_cubeVB.bind();
    m_cubeInstanceBuffer.bind(true);
    m_cubeIB.bind();
    pyr::Engine::d3dcontext().DrawIndexedInstanced(m_cubeIB.getIndicesCount(), 3, 0, 0, 0);

    pyr::RenderProfiles::popDepthProfile();
    pyr::RenderProfiles::popRasterProfile();
  }
};
}
