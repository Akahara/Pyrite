#include "DebugDraw.h"

#include <execution>

#include "engine/Engine.h"

namespace pyr
{

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

void DebugDraws::drawDebugPoint(const vec3& location, const vec4& color, float duration)
{
  m_debugPoints.emplace_back(location, color, duration);
}

void DebugDraws::drawDebugCamera(const Camera& camera)
{
}

void DebugDraws::tick(float deltaTime)
{
  std::ranges::for_each(m_debugLines, [&](auto &l) { l.remainingDuration -= deltaTime; });
  std::ranges::for_each(m_debugPoints, [&](auto &l) { l.remainingDuration -= deltaTime; });
  m_debugLines.erase(
    std::partition(std::execution::par, m_debugLines.begin(), m_debugLines.end(), [](auto &l) { return l.remainingDuration < 0; }),
    m_debugLines.end());
  m_debugPoints.erase(
    std::partition(std::execution::par, m_debugPoints.begin(), m_debugPoints.end(), [](auto &l) { return l.remainingDuration < 0; }),
    m_debugPoints.end());
}

void DebugDraws::render()
{
  if (!PYR_ENSURE(m_viewportCam))
    return;
  auto &context = Engine::d3dcontext();
  context.IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_LINELIST);
}

}
