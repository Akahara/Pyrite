#pragma once

#include "AABB.h"
#include "utils/math.h"

struct Transform
{
  vec3 position{};
  vec3 scale = vec3::One;
  quat rotation = quat::Identity;

  mat4 getWorldMatrix() const { return mat4::CreateScale(scale) * mat4::CreateFromQuaternion(rotation) * mat4::CreateTranslation(position); }
  vec3 getUp() const { return vec3::Transform(vec3::Up, rotation); }
  vec3 getForward() const { return vec3::Transform(vec3::Forward, rotation); }
  vec3 getRight() const { return vec3::Transform(vec3::Right, rotation); }

  vec3 transform(const vec3 &localPos) const { return vec3::Transform(localPos * scale, rotation) + position; }
  vec3 transformDirection(const vec3 &localDir) const { return mathf::normalize(vec3::Transform(localDir * scale, rotation)); }
  vec3 inverseTransform(const vec3 &worldPos) const { return vec3::Transform(worldPos - position, mathf::invQuat(rotation)) / scale; }
  vec3 inverseTransformDirection(const vec3 &worldDir) const { return vec3::Transform(worldDir, mathf::invQuat(rotation)) / scale; }
};
