#pragma once

#include <variant>

#include "AABB.h"
#include "Transform.h"
#include "utils/Math.h"

namespace pyr
{

struct PerspectiveProjection
{
  //float fovy   = PI * .4f;
   float fovy = 1.02974f;
  float aspect = 16.f/9.f;
  float zNear  = 0.1f;     // world space
  float zFar   = 10000.f; // world space

  mat4 buildProjectionMatrix() const;
};

struct OrthographicProjection
{
  float width  = 160;    // world space
  float height = 90;     // world space
  float zNear  = .1f;    // world space
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
  void setPosition(const vec3 &position) { m_transform.position = position; m_bViewMatrixDirty = true; }
  void setRotation(const quat &rotation) { m_transform.rotation = rotation; m_bViewMatrixDirty = true; }
  void setTransform(const Transform& transform) { m_transform = transform; m_bViewMatrixDirty = true; }
  void lookAt(const vec3 &target);

  void rotate(const quat rot) { m_transform.rotation *= rot; m_bViewMatrixDirty = true; }
  void rotate(float rx, float ry, float rz, bool absolute = false);
  void move(const vec3& dl) { m_transform.position += dl; m_bViewMatrixDirty = true; }

  const mat4 &getViewProjectionMatrix() const;
  const mat4 &getProjectionMatrix() const;
  const mat4 &getViewMatrix() const;
  const Transform& GetTransform() const { return m_transform; }
  const vec3 &getPosition() const { return m_transform.position; }
  const quat &getRotation() const { return m_transform.rotation; }

  vec3 getRight() const { return m_transform.transformDirection(vec3::Right); }
  vec3 getUp() const { return m_transform.transformDirection(vec3::Up); }
  vec3 getForward() const { return m_transform.transformDirection(vec3::Forward); }
  vec3 getFlatForward() const;

private:
  void updateViewMatrix() const;
  void updateViewProjMatrix() const;

private:
  CameraProjection m_projection;
  Transform m_transform;
  mat4 m_projectionMatrix{};
  mutable mat4 m_viewMatrix{};
  bool m_bViewMatrixDirty = true;
  mutable mat4 m_viewProjectionMatrix{};
  bool m_bViewProjectionMatrixDirty = true;
};

struct Plane
{
  vec3 normal{};
  float distanceToOrigin = 0;

  Plane() = default;

  Plane(const vec3& pl, const vec3& norm) 
  {
      normal = norm;
      normal.Normalize();
      distanceToOrigin = normal.Dot(pl);
  }

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


class FreecamController
{
public:
  explicit FreecamController(Camera* camera=nullptr)
    : m_camera(camera) {}

  void processUserInputs(double deltaTime);

  void setSpeed(float speed) { m_playerSpeed = speed; }
  const Camera *getCamera() const { return m_camera; }
  void setCamera(Camera* camera) { m_camera = camera; }

private:
  float m_playerSpeed = 15.0;
  bool  m_cursorLocked{};
  Camera* m_camera;
};

}