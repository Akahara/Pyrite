#pragma once

#include "utils/Math.h"

namespace pyr
{
class StaticMesh;

struct Ray
{
	vec3 origin;
	vec3 direction;
	float maxDistance = std::numeric_limits<float>::infinity();
};

struct RayResult
{
	bool bHit = false;
	float distance;
	vec3 position;
	vec3 normal;

	[[nodiscard]] operator bool() const noexcept { return bHit; }
};
	
RayResult raytrace(const StaticMesh &mesh, const Ray& ray);

}
