#pragma once

#include "utils/Math.h"
#include "world/camera.h"

namespace pyr
{

class DebugDraws
{
public:
  void drawDebugLine(const vec3& p0, const vec3& p1, const vec4& color, float duration);
  void drawDebugSphere(const vec3& center, float radius, const vec4& color, float duration);
  void drawDebugBox(const vec3& center, const vec3& extent, const vec4& color, float duration);
  void drawDebugPoint(const vec3& location, const vec4& color, float duration);
  void drawDebugCamera(const Camera& camera);

  void setViewportCamera(const Camera *camera) { m_viewportCam = camera; }
  void tick(float deltaTime);
  void render();

  static DebugDraws& get() { return singleton; }

private:
  static DebugDraws singleton;

  struct DebugLine
  {
    vec3 p0, p1;
    vec4 color;
    float remainingDuration;
  };

  struct DebugPoint
  {
    vec3 p;
    vec4 color;
    float remainingDuration;
  };

  std::vector<DebugLine> m_debugLines;
  std::vector<DebugPoint> m_debugPoints;
  const Camera* m_viewportCam = nullptr;
};
	
inline void drawDebugLine(const vec3& p0, const vec3& p1, const vec4& color={ .9f, 1.f, .3f, 1.f }, float duration=0.f)
{ DebugDraws::get().drawDebugLine(p0, p1, color, duration); }
inline void drawDebugSphere(const vec3& center, float radius, const vec4& color={ 1.f, .5f, .3f, 1.f }, float duration=0.f)
{ DebugDraws::get().drawDebugSphere(center, radius, color, duration); }
inline void drawDebugBox(const vec3& center, const vec3& extent, const vec4& color={ 1.f, .5f, .3f, 1.f }, float duration=0.f)
{ DebugDraws::get().drawDebugBox(center, extent, color, duration); }
inline void drawDebugPoint(const vec3& location, const vec4& color={ 1.f, .5f, .3f, 1.f }, float duration=0.f)
{ DebugDraws::get().drawDebugPoint(location, color, duration); }
inline void drawDebugCamera(const Camera& camera)
{ DebugDraws::get().drawDebugCamera(camera); }

}
