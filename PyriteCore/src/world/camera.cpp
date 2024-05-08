#include "camera.h"

using namespace DirectX;

namespace pyr
{

mat4 PerspectiveProjection::buildProjectionMatrix() const
{
  return XMMatrixPerspectiveFovLH(fovy, aspect, zNear, zFar);
}

mat4 OrthographicProjection::buildProjectionMatrix() const
{
  return XMMatrixOrthographicLH(width, height, zNear, zFar);
}

void Camera::updateViewMatrix()
{
  quat invRot; m_rotation.Inverse(invRot);
  m_viewMatrix = XMMatrixTranslationFromVector(-m_position) * XMMatrixRotationQuaternion(invRot);
  updateViewProjectionMatrix();
}

void Camera::updateViewProjectionMatrix()
{
  m_viewProjectionMatrix = m_viewMatrix * m_projectionMatrix;
}

vec3 Camera::getRight() const
{
  return XMVector3Rotate({ 1,0,0 }, m_rotation);
}

vec3 Camera::getUp() const
{
  return XMVector3Rotate({ 0,1,0 }, m_rotation);
}

vec3 Camera::getForward() const
{
  return XMVector3Rotate({ 0,0,1 }, m_rotation);
}

vec3 Camera::getFlatForward() const
{
  return XMVector3Normalize(XMVector3Cross(getRight(), vec3::Up));
}

void Camera::rotate(float rx, float ry, float rz, bool absolute)
{
  const quat rotx = XMQuaternionRotationAxis({ 1, 0, 0, 0 }, rx);
  const quat roty = XMQuaternionRotationAxis({ 0, 1, 0, 0 }, ry);
  const quat rotz = XMQuaternionRotationAxis({ 0, 0, 1, 0 }, rz);
  const quat worldRot = XMQuaternionMultiply(rotx, XMQuaternionMultiply(roty, rotz));
  if (absolute)
    m_rotation = XMQuaternionMultiply(worldRot, m_rotation);
  else
    m_rotation = XMQuaternionMultiply(m_rotation, worldRot);
}

void Camera::move(const vec3 &dl) {
  m_position = XMVectorSetW(m_position + dl, 1.f);
}

quat Camera::quaternionLookAt(const vec3& pos, const vec3& target)
{
  vec3 d = XMVector3Normalize(pos - target);
  float pitch = asinf(XMVectorGetY(d));
  float yaw = std::atan2(XMVectorGetX(d), XMVectorGetZ(d)) + XM_PI;
  return XMQuaternionMultiply(XMQuaternionRotationAxis({ 1,0,0 }, pitch), XMQuaternionRotationAxis({ 0,1,0 }, yaw));
}

void Camera::setProjection(const CameraProjection& proj)
{
  m_projection = proj;
  m_projectionMatrix = std::visit([](const auto &proj) -> mat4 { return proj.buildProjectionMatrix(); }, proj);
}

void Camera::setRotation(float yaw, float pitch)
{
  m_rotation = XMQuaternionMultiply(XMQuaternionRotationAxis({ 1,0,0 }, pitch), XMQuaternionRotationAxis({ 0,1,0 }, yaw));
}

void Camera::lookAt(const vec3 &target)
{
  m_rotation = quaternionLookAt(m_position, target);
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
  frustum.rightFace  = { cam.getPosition(), XMVector3Cross(farRay + cam.getRight() * halfHSide, cam.getUp()) };
  frustum.leftFace   = { cam.getPosition(), XMVector3Cross(cam.getUp(), farRay - cam.getRight() * halfHSide) };
  frustum.topFace    = { cam.getPosition(), XMVector3Cross(farRay - cam.getUp() * halfVSide, cam.getRight()) };
  frustum.bottomFace = { cam.getPosition(), XMVector3Cross(cam.getRight(), farRay + cam.getUp() * halfVSide) };

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
  float r = XMVectorGetX(XMVector3Dot(e, n));
  return -r <= plane.signedDistanceTo(center);
}

}