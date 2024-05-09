#pragma once

#include <variant>

#include "AABB.h"
#include "Transform.h"
#include "utils/Math.h"

namespace pyr
{

struct PerspectiveProjection
{
  float fovy   = PI * .4f;
  float aspect = 16.f/9.f;
  float zNear  = 3.f;     // world space
  float zFar   = 10000.f; // world space

  mat4 buildProjectionMatrix() const;
};

struct OrthographicProjection
{
  float width  = 160;    // world space
  float height = 90;     // world space
  float zNear  = 3.f;    // world space
  float zFar = 10000.f;  // world space

  mat4 buildProjectionMatrix() const;
};

using CameraProjection = std::variant<PerspectiveProjection, OrthographicProjection>;


class Camera
{
public:
  Camera() = default;

  const CameraProjection &getProjection() const { return m_projection; }
  void setProjection(const CameraProjection &proj);
  void setPosition(const vec3 &position) { m_transform.position = position; }
  void setRotation(const quat &rotation) { m_transform.rotation = rotation; }
  void setTransform(const Transform& transform) { m_transform = transform; }
  void lookAt(const vec3 &target);

  void rotate(const quat rot) { m_transform.rotation *= rot; }
  void move(const vec3& dl) { m_transform.position += dl; }

  // must be called after a position/rotation update !
  void updateViewMatrix();
  // must be called after a view/projection update !
  void updateViewProjectionMatrix();

  const mat4 &getViewProjectionMatrix() const { return m_viewProjectionMatrix; }
  const mat4 &getProjectionMatrix() const { return m_projectionMatrix; }
  const mat4 &getViewMatrix() const { return m_viewMatrix; }
  const Transform& getTransform() const { return m_transform; }
  const vec3 &getPosition() const { return m_transform.position; }
  const quat &getRotation() const { return m_transform.rotation; }

  vec3 getRight() const { return m_transform.rotate(vec3::Right); }
  vec3 getUp() const { return m_transform.rotate(vec3::Up); }
  vec3 getForward() const { return m_transform.rotate(vec3::Forward); }
  vec3 getFlatForward() const;

private:
  CameraProjection m_projection;
  Transform m_transform;
  mat4  m_projectionMatrix{};
  mat4  m_viewMatrix{};
  mat4  m_viewProjectionMatrix{};
};

struct Plane
{
  vec3 normal{};
  float distanceToOrigin = 0;

  Plane() = default;

  Plane(const vec3& pl, const vec3 norm) 
    : normal(XMVector3Normalize(norm))
    , distanceToOrigin(XMVectorGetX(XMVector3Dot(normal, pl)))
  {}

  float signedDistanceTo(const vec3& point) const
  {
    return normal.Dot(point) - distanceToOrigin;
  }
};

/*
 * A frustum is the world region that is visible to a camera.
 * Frustum intersections can be checked against AABBs and such to do frustum culling.
 */
struct Frustum
{
  Plane topFace;
  Plane bottomFace;
  Plane rightFace;
  Plane leftFace;
  Plane farFace;
  Plane nearFace;

  bool isOnFrustum(const AABB &boudingBox) const;

  static Frustum createFrustumFromCamera(const Camera &cam);
  static Frustum createFrustumFromProjection(const Camera &cam, const OrthographicProjection &proj);
  static Frustum createFrustumFromProjection(const Camera &cam, const PerspectiveProjection &proj);

  static bool isOnOrForwardPlan(const Plane &plane, const AABB &boundingBox);
};


}