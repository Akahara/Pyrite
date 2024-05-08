#include "camera.h"

using namespace DirectX;

namespace pyr
{

mat4 PerspectiveProjection::buildProjectionMatrix() const
{
  return mat4::CreatePerspectiveFieldOfView(fovy, aspect, zNear, zFar);
}

mat4 OrthographicProjection::buildProjectionMatrix() const
{
  return mat4::CreateOrthographic(width, height, zNear, zFar);
}

void Camera::updateViewMatrix()
{
  quat invRot; m_transform.rotation.Inverse(invRot);
  m_viewMatrix = mat4::CreateTranslation(-m_transform.position) * mat4::CreateFromQuaternion(invRot);
  updateViewProjectionMatrix();
}

void Camera::updateViewProjectionMatrix()
{
  m_viewProjectionMatrix = m_viewMatrix * m_projectionMatrix;
}

vec3 Camera::getFlatForward() const
{
  vec3 c = getRight().Cross(vec3::Up);
  c.Normalize();
  return c;
}

void Camera::setProjection(const CameraProjection& proj)
{
  m_projection = proj;
  m_projectionMatrix = std::visit([](const auto &proj) -> mat4 { return proj.buildProjectionMatrix(); }, proj);
}

void Camera::lookAt(const vec3 &target)
{
  m_transform.rotation = quat::LookRotation(target - m_transform.position, vec3::Up);
}

Frustum Frustum::createFrustumFromProjection(const Camera &cam, const OrthographicProjection &proj)
{
  Frustum frustum;

  vec3 forward = cam.getForward();
  vec3 right   = cam.getRight();
  vec3 up      = cam.getUp();
  vec3 pos     = cam.getPosition();
  frustum.nearFace   = { pos + proj.zNear * forward, cam.getForward() };
  frustum.farFace    = { pos + proj.zFar * forward, -cam.getForward() };
  frustum.rightFace  = { pos + right * proj.width*.5f,  right };
  frustum.leftFace   = { pos - right * proj.width*.5f, -right };
  frustum.topFace    = { pos + up * proj.height*.5f,  up };
  frustum.bottomFace = { pos - up * proj.height*.5f, -up };

  return frustum;
}

Frustum Frustum::createFrustumFromProjection(const Camera &cam, const PerspectiveProjection &proj)
{
  Frustum frustum;

  float halfVSide = proj.zFar * tanf(proj.fovy * .5f);
  float halfHSide = halfVSide * proj.aspect;
  vec3 farRay     = proj.zFar * cam.getForward();
  frustum.nearFace   = { cam.getPosition() + proj.zNear * cam.getForward(), cam.getForward() };
  frustum.farFace    = { cam.getPosition() + farRay, -cam.getForward() };
  frustum.rightFace  = { cam.getPosition(), (farRay + cam.getRight() * halfHSide).Cross(cam.getUp()) };
  frustum.leftFace   = { cam.getPosition(), cam.getUp().Cross(farRay - cam.getRight() * halfHSide) };
  frustum.topFace    = { cam.getPosition(), (farRay - cam.getUp() * halfVSide).Cross(cam.getRight()) };
  frustum.bottomFace = { cam.getPosition(), cam.getRight().Cross(farRay + cam.getUp() * halfVSide) };

  return frustum;
}

bool Frustum::isOnFrustum(const AABB &boudingBox) const
{
  return 
    isOnOrForwardPlan(leftFace,   boudingBox) &&
    isOnOrForwardPlan(rightFace,  boudingBox) &&
    isOnOrForwardPlan(topFace,    boudingBox) &&
    isOnOrForwardPlan(bottomFace, boudingBox) &&
    isOnOrForwardPlan(nearFace,   boudingBox) &&
    isOnOrForwardPlan(farFace,    boudingBox) &&
    true;
}

Frustum Frustum::createFrustumFromCamera(const Camera& cam)
{
  return std::visit([&](const auto &proj) { return createFrustumFromProjection(cam, proj); }, cam.getProjection());
}

bool Frustum::isOnOrForwardPlan(const Plane &plane, const AABB &boundingBox)
{
  vec3 center = boundingBox.getOrigin() + boundingBox.getSize() / 2.f;
  vec3 e = boundingBox.getOrigin() + boundingBox.getSize() - center;
  vec3 n = XMVectorAbs(plane.normal);
  float r = e.Dot(n);
  return -r <= plane.signedDistanceTo(center);
}

}