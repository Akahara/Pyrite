#pragma once

#include "utils/Math.h"

namespace pyr
{
class StaticMesh;

struct Ray
{
	vec3 origin;
	vec3 direction;
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
