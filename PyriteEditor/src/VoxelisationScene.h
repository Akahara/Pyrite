#pragma once

#include "display/DebugDraw.h"
#include "display/IndexBuffer.h"
#include "display/InputLayout.h"
#include "display/Vertex.h"
#include "display/VertexBuffer.h"
#include "display/RenderGraph/RenderGraph.h"
#include "engine/Engine.h"
#include "scene/Scene.h"
#include "world/camera.h"
#include "display/GraphicalResource.h"
#include "display/RenderProfiles.h"

namespace pye {

struct ivec3
{
  int32_t x{}, y{}, z{};

  static ivec3 floor(const vec3 &v) { return { static_cast<int32_t>(v.x), static_cast<int32_t>(v.y), static_cast<int32_t>(v.z) }; }

  operator vec3() const { return { static_cast<float>(x),static_cast<float>(y),static_cast<float>(z) }; }
};

template<class Cell>
class VoxelGrid
{
public:
  explicit VoxelGrid(const Transform& transform, const ivec3& dimensions)
    : m_transform(transform)
    , m_dimensions(dimensions)
    , m_cells(dimensions.x * dimensions.y * dimensions.z)
  {
    PYR_ASSERT(dimensions.x >= 0 && dimensions.y >= 0 && dimensions.z >= 0);
  }

  vec3 cellToWorld(const ivec3& cell) const
  {
    return m_transform.transform((cell + vec3(.5f)) / m_dimensions - vec3(.5f));
  }

  ivec3 worldToCell(const vec3& world) const
  {
    return ivec3::floor(m_transform.inverseTransform(world));
  }

  bool isValidCell(const ivec3& cell) const
  {
    return cell.x >= 0 && cell.x < m_dimensions.x
        && cell.y >= 0 && cell.y < m_dimensions.y
        && cell.z >= 0 && cell.z < m_dimensions.z;
  }

  Cell& operator[](const ivec3& cell) requires !std::is_same_v<bool, Cell>
  {
    PYR_ASSERT(isValidCell(cell));
    return m_cells[cell.x + m_dimensions.x * (cell.y + m_dimensions.y * cell.z)];
  }

  auto operator[](const ivec3& cell) requires std::is_same_v<bool, Cell>
  {
    PYR_ASSERT(isValidCell(cell));
    return m_cells[cell.x + m_dimensions.x * (cell.y + m_dimensions.y * cell.z)];
  }

  Cell& at(int32_t x, int32_t y, int32_t z) requires !std::is_same_v<bool, Cell>
  {
    return (*this)[{x, y, z}];
  }

  auto at(int32_t x, int32_t y, int32_t z) requires std::is_same_v<bool, Cell>
  {
    return (*this)[{x, y, z}];
  }

  const Transform& getTransform() const
  {
    return m_transform;
  }

  const ivec3& getDimensions() const
  {
    return m_dimensions;
  }

  Transform getCellTransform(ivec3 cell) const
  {
    Transform t{
      .position = cellToWorld(cell),
      .scale = m_transform.scale / m_dimensions,
      .rotation = m_transform.rotation,
    };
    return t;
  }

private:
  Transform m_transform;
  ivec3 m_dimensions;
  std::vector<Cell> m_cells;
};

using CameraBuffer = pyr::ConstantBuffer<InlineStruct(mat4 mvp; alignas(sizeof vec4) vec3 pos)>;

class InstancedCube
{
public:
  using CubeVertex = pyr::GenericVertex<pyr::POSITION>;
  using CubeInstance = pyr::GenericVertex<pyr::INSTANCE_TRANSFORM, pyr::INSTANCE_COLOR>;

public:
  InstancedCube(pyr::GraphicalResourceRegistry& grr, std::shared_ptr<CameraBuffer> cameraBuf)
  {
    std::vector<CubeVertex> cubeVertices{ {
      { vec4(-.5f, -.5f, -.5f, 1) },
      { vec4(+.5f, -.5f, -.5f, 1) },
      { vec4(+.5f, +.5f, -.5f, 1) },
      { vec4(-.5f, +.5f, -.5f, 1) },
      { vec4(-.5f, -.5f, +.5f, 1) },
      { vec4(+.5f, -.5f, +.5f, 1) },
      { vec4(+.5f, +.5f, +.5f, 1) },
      { vec4(-.5f, +.5f, +.5f, 1) },
    } };
    std::vector<pyr::IndexBuffer::size_type> cubeIndices{ {
      0, 3, 1, 1, 3, 2,
      1, 2, 5, 5, 2, 6,
      5, 6, 4, 4, 6, 7,
      4, 7, 0, 0, 7, 3,
      3, 7, 2, 2, 7, 6,
      4, 0, 5, 5, 0, 1,
    } };
    m_cubeIB = pyr::IndexBuffer(cubeIndices);
    m_cubeVB = pyr::VertexBuffer(cubeVertices);
    m_baseEffect = grr.loadEffect(L"res/shaders/debug_cubes.fx", pyr::InputLayout::MakeLayoutFromVertex<CubeVertex, CubeInstance>());
    m_baseEffect->addBinding({ .label = "CameraBuffer", .bufferRef = cameraBuf });
  }

  void render()
  {
    if (m_cubeInstancesDirty) {
      if (m_cubeInstanceBufferSize >= cubeInstances.size())
        m_cubeInstanceBuffer.setData(cubeInstances.data(), sizeof(CubeInstance) * cubeInstances.size(), 0);
      else
        m_cubeInstanceBuffer = pyr::VertexBuffer(cubeInstances, true);
      m_cubeInstanceBufferSize = cubeInstances.size();
      m_cubeInstancesDirty = false;
    } else {
      PYR_ENSURE(m_cubeInstanceBufferSize == cubeInstances.size(), "You forgot to call markInstancesDirty after updating instances and before rendering");
    }
    if (m_cubeInstanceBufferSize == 0) return;
    m_baseEffect->uploadAllBindings();
    m_baseEffect->bind();
    m_cubeVB.bind();
    m_cubeInstanceBuffer.bind(true);
    m_cubeIB.bind();
    pyr::Engine::d3dcontext().DrawIndexedInstanced(
      static_cast<UINT>(m_cubeIB.getIndicesCount()),
      static_cast<UINT>(m_cubeInstanceBufferSize),
      0, 0, 0);
  }

public:
  std::vector<CubeInstance> cubeInstances;
  void markInstancesDirty() { m_cubeInstancesDirty = true; }

private:
  pyr::VertexBuffer m_cubeVB;
  pyr::VertexBuffer m_cubeInstanceBuffer;
  pyr::IndexBuffer m_cubeIB;
  pyr::Effect *m_baseEffect;

  size_t m_cubeInstanceBufferSize = 0;
  bool m_cubeInstancesDirty = false;
};

class VoxelisationDemoScene : public pyr::Scene {
private:
  pyr::Camera m_camera;
  pyr::FreecamController m_camController;

  std::shared_ptr<CameraBuffer> m_cameraBuffer = std::make_shared<CameraBuffer>();
  pyr::GraphicalResourceRegistry m_grr;
  InstancedCube m_cubes;

  pyr::RenderGraph m_RDG;
  pyr::BuiltinPasses::ForwardPass m_forwardPass;
  pyr::Mesh m_meshMesh;
  pyr::Model m_meshModel;
  pyr::StaticMesh m_mesh;

  VoxelGrid<bool> m_voxelGrid;

public:
  VoxelisationDemoScene()
    : m_cubes(m_grr, m_cameraBuffer)
    , m_voxelGrid(Transform{ vec3::Zero, vec3(5.f), quat::Identity }, ivec3{ 10,10,10 })
  {
    m_camera.setProjection(pyr::PerspectiveProjection{});
    m_camController.setCamera(&m_camera);
    drawDebugSetCamera(&m_camera);

    fs::path meshFile = "res/meshes/axes.obj";
    pyr::Effect* meshEffect = m_grr.loadEffect(L"res/shaders/mesh.fx", pyr::InputLayout::MakeLayoutFromVertex<pyr::Mesh::mesh_vertex_t>());
    meshEffect->addBinding({ .label = "CameraBuffer", .bufferRef = m_cameraBuffer });
    m_forwardPass.getSkyboxEffect()->addBinding({ .label = "CameraBuffer", .bufferRef = m_cameraBuffer });
    m_meshMesh = pyr::MeshImporter::ImportMeshFromFile(meshFile);
    m_meshModel = pyr::Model{ m_meshMesh };
    m_mesh = pyr::StaticMesh{ m_meshModel };
    m_mesh.setBaseMaterial(std::make_shared<pyr::Material>(meshEffect));
    m_mesh.loadSubmeshesMaterial(pyr::MeshImporter::FetchMaterialPaths(meshFile));
    m_RDG.addPass(&m_forwardPass);
    m_forwardPass.addMeshToPass(&m_mesh);

    voxeliseMesh();
    updateCubesToMatchVoxelGrid();
  }

  void update(float delta) override
  {
    m_camController.processUserInputs(delta);
  }

  void voxeliseMesh()
  {
    ivec3 dims = m_voxelGrid.getDimensions();
    ivec3 p;
    vec3 d{ 1,2,3 };
    d.Normalize();
    for (p.x = 0; p.x < dims.x; p.x++)
    for (p.y = 0; p.y < dims.y; p.y++)
    for (p.z = 0; p.z < dims.z; p.z++)
    {
      pyr::Ray ray{ m_voxelGrid.cellToWorld(p), d, 10.f };
      pyr::RayResult result = pyr::raytrace(m_mesh, ray);
      //pyr::drawDebugLine(
      //  ray.origin,
      //  ray.origin + ray.direction * (result.bHit ? result.distance : ray.maxDistance),
      //  result.bHit ? vec4(0, 1, 0, 1) : vec4(1, 0, 0, 1),
      //  15.f);
      //if (result.bHit)
      //  pyr::drawDebugLine(result.position, result.position + result.normal, vec4{ 1,1,.2f,1.f }, 15.f);
      m_voxelGrid[p] = result.bHit && (result.normal.Dot(d) > 0);
    }
  }

  void updateCubesToMatchVoxelGrid()
  {
    ivec3 dims = m_voxelGrid.getDimensions();
    m_cubes.cubeInstances.clear();
    m_cubes.cubeInstances.reserve(dims.x * dims.y * dims.z);
    size_t count = 0;
    ivec3 p;
    auto rng = mathf::randomFunction();
    for (p.x = 0; p.x < dims.x; p.x++)
    for (p.y = 0; p.y < dims.y; p.y++)
    for (p.z = 0; p.z < dims.z; p.z++)
    {
      if (m_voxelGrid[p]) {
        m_cubes.cubeInstances.push_back(InstancedCube::CubeInstance{
          m_voxelGrid.getCellTransform(p).getWorldMatrix(),
          vec4(1,rng(0,.4f),rng(0,.4f),.2f)
        });
        count++;
      }
    }
    m_cubes.markInstancesDirty();
  }

  void render() override
  {
    ImGui::Begin("Voxelization");
    static ivec3 dims = m_voxelGrid.getDimensions();
    static vec3 position = m_voxelGrid.getTransform().position;
    static vec3 scale = m_voxelGrid.getTransform().scale;
    static bool autogen = true;
    static bool showMesh = true;
    ImGui::Checkbox("Autogen", &autogen);
    if (ImGui::Checkbox("ShowMesh", &showMesh)) {
      m_forwardPass.clear();
      if (showMesh) m_forwardPass.addMeshToPass(&m_mesh);
    }
    if ((ImGui::DragInt3("Dimensions", &dims.x, 1, 1, 100)
        + ImGui::DragFloat3("Position", &position.x)
        + ImGui::DragFloat3("Scale", &scale.x)) * autogen
        + ImGui::Button("Regenerate")
        ) {
      m_voxelGrid = VoxelGrid<bool>{ Transform{ position, scale }, dims };
      voxeliseMesh();
      updateCubesToMatchVoxelGrid();
    }
    ImGui::End();

    m_cameraBuffer->setData(CameraBuffer::data_t{ .mvp = m_camera.getViewProjectionMatrix(), .pos = m_camera.getPosition() });
    pyr::RenderProfiles::pushRasterProfile(pyr::RasterizerProfile::CULLBACK_RASTERIZER);
    m_RDG.execute();
    pyr::RenderProfiles::popRasterProfile();

    pyr::RenderProfiles::pushBlendProfile(pyr::BlendProfile::BLEND);
    pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTONLY_DEPTH);
    m_cubes.render();
    pyr::RenderProfiles::popBlendProfile();
    pyr::RenderProfiles::popDepthProfile();

    pyr::drawDebugBox(m_voxelGrid.getTransform());
  }
};
}
