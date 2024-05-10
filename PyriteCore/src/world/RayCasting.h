#pragma once

#include "Mesh/Mesh.h"
#include "utils/Math.h"

namespace pyr
{

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
	
RayResult raytrace(const Mesh& mesh, const Ray& ray);

}
