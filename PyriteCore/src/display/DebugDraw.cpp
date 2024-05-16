#include "DebugDraw.h"

#include <execution>

#include "GraphicalResource.h"
#include "VertexBuffer.h"
#include "engine/Engine.h"
#include "utils/Utils.h"

namespace pyr
{

DebugDraws DebugDraws::s_singleton;

void DebugDraws::drawDebugLine(const vec3& p0, const vec3& p1, const vec4& color, float duration)
{
  m_debugLines.emplace_back(p0, p1, color, duration);
}

void DebugDraws::drawDebugSphere(const vec3& center, float radius, const vec4& color, float duration)
{
  constexpr int N = 8, M = 6;

  for (int j = 1; j < M; j++) {
    float a = PI * j / M;
    float y = std::cos(a);
    float r = std::sin(a);
    for (int i = 0; i < N; i++) {
      float t0 = PI * 2.f * i / N;
      float t1 = PI * 2.f * (i + 1) / N;
      m_debugLines.emplace_back(
        center + vec3{ std::cos(t0) * r, y, std::sin(t0) * r } * radius,
        center + vec3{ std::cos(t1) * r, y, std::sin(t1) * r } * radius,
        color,
        duration
      );
    }
  }

  for (int i = 0; i < N; i++) {
    float t = PI * 2.f * i / N;
    float x = std::cos(t);
    float z = std::sin(t);
    for (int j = 0; j < M; j++) {
      float a0 = PI * j / M;
      float a1 = PI * (j+1) / M;
      m_debugLines.emplace_back(
        center + vec3{ x * std::sin(a0), std::cos(a0), z * std::sin(a0) } *radius,
        center + vec3{ x * std::sin(a1), std::cos(a1), z * std::sin(a1) } *radius,
        color,
        duration
      );
    }
  }
}

void DebugDraws::drawDebugBox(const vec3& center, const vec3& extent, const vec4& color, float duration)
{
  vec3 a = center - extent * .5f;
  constexpr float shifter[]{ 0,0,1,0,0,1,0,0 };
  for (int i = 0; i < 3; i++) {
    auto& d = reinterpret_cast<const vec3&>(shifter[i]);
    auto& u = reinterpret_cast<const vec3&>(shifter[i+1]);
    auto& v = reinterpret_cast<const vec3&>(shifter[i+2]);
    m_debugLines.emplace_back(a,                  a + extent * d,       color, duration);
    m_debugLines.emplace_back(a + extent * u,     a + extent * (d+u),   color, duration);
    m_debugLines.emplace_back(a + extent * v,     a + extent * (d+v),   color, duration);
    m_debugLines.emplace_back(a + extent * (u+v), a + extent * (d+u+v), color, duration);
  }
}

void DebugDraws::drawDebugBox(const Transform& transform, const vec4& color, float duration)
{
  vec3 a(-.5f);
  constexpr float shifter[]{ 0,0,1,0,0,1,0,0 };
  for (int i = 0; i < 3; i++) {
    auto &d = reinterpret_cast<const vec3&>(shifter[i]);
    auto &u = reinterpret_cast<const vec3&>(shifter[i + 1]);
    auto &v = reinterpret_cast<const vec3&>(shifter[i + 2]);
    m_debugLines.emplace_back(transform.transform(a),           transform.transform(a + d),           color, duration);
    m_debugLines.emplace_back(transform.transform(a + u),       transform.transform(a + (d + u)),     color, duration);
    m_debugLines.emplace_back(transform.transform(a + v),       transform.transform(a + (d + v)),     color, duration);
    m_debugLines.emplace_back(transform.transform(a + (u + v)), transform.transform(a + (d + u + v)), color, duration);
  }
}

void DebugDraws::drawDebugPoint(const vec3& location, const vec4& color, float duration)
{
  m_debugPoints.emplace_back(location, color, duration);
}

void DebugDraws::drawDebugCamera(const Camera& camera)
{
  vec4 color{ 0.7f , 0.8f, 0.8f, 1.f };
  vec3 pos = camera.getPosition();
  vec3 I = camera.getRight();
  vec3 J = camera.getUp();
  vec3 F = camera.getForward();

  std::visit(Overloaded{
    [&](const PerspectiveProjection& proj)
    {
      float dh = tanf(proj.fovy * .5f);
      float dw = dh * proj.aspect;
      vec3 U1 = F + dh * J + dw * I;
      vec3 U2 = F - dh * J + dw * I;
      vec3 U3 = F - dh * J - dw * I;
      vec3 U4 = F + dh * J - dw * I;
      float zNear = proj.zNear;
      float zFar = proj.zFar;

      m_debugLines.emplace_back(pos, pos + U1 * zFar, color, 0.f);
      m_debugLines.emplace_back(pos, pos + U2 * zFar, color, 0.f);
      m_debugLines.emplace_back(pos, pos + U3 * zFar, color, 0.f);
      m_debugLines.emplace_back(pos, pos + U4 * zFar, color, 0.f);
      m_debugLines.emplace_back(pos + U1 * zFar, pos + U2 * zFar, color, 0.f);
      m_debugLines.emplace_back(pos + U2 * zFar, pos + U3 * zFar, color, 0.f);
      m_debugLines.emplace_back(pos + U3 * zFar, pos + U4 * zFar, color, 0.f);
      m_debugLines.emplace_back(pos + U4 * zFar, pos + U1 * zFar, color, 0.f);
      for (float z = zNear; z < zFar; z += 5.f) {
        m_debugLines.emplace_back(pos + U1 * z, pos + U2 * z, color, 0.f);
        m_debugLines.emplace_back(pos + U2 * z, pos + U3 * z, color, 0.f);
        m_debugLines.emplace_back(pos + U3 * z, pos + U4 * z, color, 0.f);
        m_debugLines.emplace_back(pos + U4 * z, pos + U1 * z, color, 0.f);
      }
    },
    [&](const OrthographicProjection& proj)
    {
      float zNear = proj.zNear;
      float zFar = proj.zFar;
      vec3 p1 = pos + I * proj.width * .5f + J * proj.height * .5f;
      vec3 p2 = pos + I * proj.width * .5f - J * proj.height * .5f;
      vec3 p3 = pos - I * proj.width * .5f - J * proj.height * .5f;
      vec3 p4 = pos - I * proj.width * .5f + J * proj.height * .5f;

      m_debugLines.emplace_back(p1, p1 + F * zFar, color, 0.f);
      m_debugLines.emplace_back(p2, p2 + F * zFar, color, 0.f);
      m_debugLines.emplace_back(p3, p3 + F * zFar, color, 0.f);
      m_debugLines.emplace_back(p4, p4 + F * zFar, color, 0.f);
      for (float z = zNear; z < zFar; z += 5.f) {
        m_debugLines.emplace_back(p1 + z * F, p2 + z * F, color, 0.f);
        m_debugLines.emplace_back(p2 + z * F, p3 + z * F, color, 0.f);
        m_debugLines.emplace_back(p3 + z * F, p4 + z * F, color, 0.f);
        m_debugLines.emplace_back(p4 + z * F, p1 + z * F, color, 0.f);
      }
    }
  }, camera.getProjection());
}

void DebugDraws::load(GraphicalResourceRegistry& resources)
{
  m_lineEffect = resources.loadEffect(L"res/shaders/debug_lines.fx", InputLayout::MakeLayoutFromVertex<Vertex>());
  m_cameraCBO = std::make_unique<CameraConstantBuffer>();
}

void DebugDraws::tick(float deltaTime)
{
  std::ranges::for_each(m_debugLines, [&](auto &l) { l.remainingDuration -= deltaTime; });
  std::ranges::for_each(m_debugPoints, [&](auto &l) { l.remainingDuration -= deltaTime; });
  m_debugLines.erase(
    std::partition(std::execution::par, m_debugLines.begin(), m_debugLines.end(), [](auto &l) { return l.remainingDuration > 0; }),
    m_debugLines.end());
  m_debugPoints.erase(
    std::partition(std::execution::par, m_debugPoints.begin(), m_debugPoints.end(), [](auto &l) { return l.remainingDuration > 0; }),
    m_debugPoints.end());
}

void DebugDraws::render()
{
  if (m_debugLines.empty() && m_debugPoints.empty())
    return;
  if (!PYR_ENSURE(m_viewportCam, "No debug viewport camera set, did you forget to call drawDebugSetCamera ?"))
    return;

  // Draw lines
  if (!m_debugLines.empty()) {
    m_lineVertexCache.clear();
    for (const DebugLine& line : m_debugLines) {
      m_lineVertexCache.push_back({ BaseVertex::toPosition(line.p0), line.color });
      m_lineVertexCache.push_back({ BaseVertex::toPosition(line.p1), line.color });
    }
    if (!m_lineVBO || m_lineVBO->getVerticesCount() < m_lineVertexCache.size()) {
      m_lineVBO = std::make_unique<VertexBuffer>(m_lineVertexCache, true);
    } else {
      m_lineVBO->setData(m_lineVertexCache.data(), sizeof(Vertex) * m_lineVertexCache.size(), 0);
    }
      
    m_cameraCBO->setData({ m_viewportCam->getViewProjectionMatrix() });

    auto &context = Engine::d3dcontext();
    context.IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_LINELIST);
    m_lineEffect->bind();
    m_lineEffect->bindConstantBuffer("cbCamera", *m_cameraCBO);
    m_lineVBO->bind();
    context.Draw(static_cast<UINT>(m_lineVertexCache.size()), 0);
  }

  // TODO Draw points
  PYR_ENSURE(m_debugPoints.empty(), "Debug points are not supported yet");
}

}
