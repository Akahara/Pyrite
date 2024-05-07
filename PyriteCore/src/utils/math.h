#pragma once

#define NOMINMAX
#include <algorithm>
#include <utility>
#include <concepts>
#include <random>
#include <cmath>

#define _XM_NO_INTRINSICS_
#include <DirectXMath.h>
#include <directtk/SimpleMath.h>

using vec2 = DirectX::SimpleMath::Vector2;
using vec3 = DirectX::SimpleMath::Vector3;
using vec4 = DirectX::SimpleMath::Vector4;
using quat = DirectX::SimpleMath::Quaternion;
using mat4 = DirectX::SimpleMath::Matrix;

static constexpr float PI = 3.14159265359f;

namespace mathf
{

template<class T> requires std::integral<T>
constexpr T ceilDivisibleBy(const T &amount, const T &divisibleBy) {
  return (amount + divisibleBy - 1) / divisibleBy * divisibleBy;
}

template<class T, class K>
constexpr T lerp(const T &t1, const T &t2, const K &t) {
  return t1 * (1-t) + t2 * t;
}

template<class T>
constexpr T inverseLerp(const T &t1, const T &t2, const T &t) {
  return (t-t1)/(t2-t1);
}

template<class T>
constexpr T clamp(const T &t, const T &min=0, const T &max=1) {
  return std::min(max, std::max(min, t));
}

template<class T>
  requires std::is_integral_v<T>
constexpr size_t firstBitIndex(T bitset) {
  if (bitset == 0)
    return -1;
  for (unsigned char i = 0; i < sizeof(T) * CHAR_BIT; i++) {
    if (bitset & (T(1) << i))
      return i;
  }
  return -1; // unreachable
}

template<class T>
  requires std::is_integral_v<T>
constexpr T positiveModulo(T n, T m) {
  return (n%m + m) % m;
}

template<class T>
  requires std::is_floating_point_v<T>
constexpr T floor(T x) {
  return std::floor(x);
}

template<class T>
  requires std::is_floating_point_v<T>
constexpr T fract(T x) {
  return x - std::floor(x);
}

template<class T, size_t N, class Generator>
  requires std::is_invocable_r_v<T, Generator, size_t>
constexpr std::array<T, N> generateLookupTable(Generator gen) {
  std::array<T, N> table;
  for (size_t i = 0; i < N; i++) table[i] = gen(i);
  return table;
}

template<class T, class K>
constexpr T smoothstep(T e0, T e1, K x) {
  auto t = clamp(inverseLerp(e0, e1, x));
  return t*t*(3-2*t);
}

inline auto randomFunction() {
  return [rd = std::mt19937{ std::random_device{}() }]() mutable {
    return inverseLerp(static_cast<float>(std::mt19937::min()), static_cast<float>(std::mt19937::max()), static_cast<float>(rd()));
  };
}

}
