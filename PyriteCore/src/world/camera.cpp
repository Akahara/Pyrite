#include "camera.h"
#include "inputs/UserInputs.h"

using namespace DirectX;

namespace pyr
{

mat4 PerspectiveProjection::buildProjectionMatrix() const
{
  mat4 R;
  XMStoreFloat4x4(&R, XMMatrixPerspectiveFovLH(fovy, aspect, zNear, zFar));
  return R;
}

mat4 OrthographicProjection::buildProjectionMatrix() const
{
  return mat4::CreateOrthographic(width, height, zNear, zFar);
}

void Camera::setProjection(const CameraProjection& proj)
{
  m_projection = proj;
  m_projectionMatrix = std::visit([](const auto &proj) -> mat4 { return proj.buildProjectionMatrix(); }, proj);
  m_bViewProjectionMatrixDirty = true;
}

void Camera::lookAt(const vec3 &target)
{
  vec3 d = m_transform.position - target;
  d.Normalize();
  float pitch = std::asinf(d.y);
  float yaw = std::atan2(d.x, d.z) + PI;
  m_transform.rotation = quat::CreateFromYawPitchRoll(yaw, pitch, 0);
  m_bViewMatrixDirty = true;
}

void Camera::rotate(float rx, float ry, float rz, bool absolute)
{
  const quat rotx = quat::CreateFromAxisAngle(vec3::UnitX, rx);
  const quat roty = quat::CreateFromAxisAngle(vec3::UnitY, ry);
  const quat rotz = quat::CreateFromAxisAngle(vec3::UnitZ, rz);
  const quat worldRot = rotx * roty * rotz;
  if (absolute)
    m_transform.rotation = worldRot * m_transform.rotation;
  else
    m_transform.rotation = m_transform.rotation * worldRot;
  m_bViewMatrixDirty = true;
}

const mat4& Camera::getViewProjectionMatrix() const
{
  if (m_bViewMatrixDirty)
  {
    updateViewMatrix();
    updateViewProjMatrix();
  }
  else if(m_bViewProjectionMatrixDirty)
  {
    updateViewProjMatrix();
  }

  return m_viewProjectionMatrix;
}

const mat4& Camera::getProjectionMatrix() const
{
  return m_projectionMatrix;
}

const mat4& Camera::getViewMatrix() const
{
  if (m_bViewMatrixDirty)
    updateViewMatrix();
  return m_viewMatrix;
}

vec3 Camera::getFlatForward() const
{
  vec3 c = getRight().Cross(vec3::Up);
  c.Normalize();
  return c;
}

void Camera::updateViewMatrix() const
{
  quat invRot; m_transform.rotation.Inverse(invRot);
  m_viewMatrix = mat4::CreateTranslation(-m_transform.position) * mat4::CreateFromQuaternion(invRot);
}

void Camera::updateViewProjMatrix() const
{
  m_viewProjectionMatrix = m_viewMatrix * m_projectionMatrix;
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

void FreecamController::processUserInputs(float delta)
{
  if (!m_camera) return;
  m_inputCooldown -= delta;

  constexpr float mouseSensitivity = 1 / 1000.f;

  const MouseState mouse = UserInputs::getMouseState();

  if (m_cursorLocked && (mouse.deltaX != 0 || mouse.deltaY != 0 || mouse.deltaScroll != 0)) {
    m_camera->rotate(0, static_cast<float>(mouse.deltaX) * mouseSensitivity, 0, false);
    m_camera->rotate(static_cast<float>(mouse.deltaY) * mouseSensitivity, 0, 0, true);
  }

  vec3 movementDelta;
  if (UserInputs::isKeyPressed(keys::SC_W)) movementDelta += m_camera->getFlatForward();
  if (UserInputs::isKeyPressed(keys::SC_A)) movementDelta -= m_camera->getRight();
  if (UserInputs::isKeyPressed(keys::SC_S)) movementDelta -= m_camera->getFlatForward();
  if (UserInputs::isKeyPressed(keys::SC_D)) movementDelta += m_camera->getRight();
  if (UserInputs::isKeyPressed(keys::SC_LEFT_SHIFT)) movementDelta += vec3::Up;
  if (UserInputs::isKeyPressed(keys::SC_LEFT_CTRL))  movementDelta -= vec3::Up;

  m_camera->move(delta * m_playerSpeed * movementDelta);

  if (mouse.deltaScroll != 0) {
    m_playerSpeed *= pow(1.1f, mouse.deltaScroll < 0 ? -1.f : +1.f);
    m_playerSpeed = mathf::clamp(m_playerSpeed, .1f, 1000.f);
  }

  if (UserInputs::isKeyPressed(keys::SC_F) && m_inputCooldown < 0) {
    UserInputs::setCursorLocked(m_cursorLocked = !m_cursorLocked);
    m_inputCooldown = .4f;
  }
}

}