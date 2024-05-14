#include "RayCasting.h"

#include "Mesh/StaticMesh.h"
#include "utils/Debug.h"

namespace pyr {

static constexpr bool bUseMeshNormals = true;

RayResult raytrace(const StaticMesh& mesh, const Ray& ray)
{
  const std::vector<Mesh::mesh_indice_t>& indices = mesh.getModel()->getRawMeshData()->getIndices();
  const std::vector<Mesh::mesh_vertex_t>& vertices = mesh.getModel()->getRawMeshData()->getVertices();
  const Transform& meshTransform = mesh.getTransform();

  RayResult result{ .distance = std::numeric_limits<float>::max() };

  PYR_ASSERT(indices.size() % 3 == 0, "The mesh is not composed of triangles");

  vec3 rayOrigin = meshTransform.inverseTransform(ray.origin);
  vec3 rayDirection = meshTransform.inverseTransformDirection(ray.direction);

  for (size_t i = 0; i < indices.size(); i += 3) {
    // https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
    constexpr float epsilon = std::numeric_limits<float>::epsilon();
    vec3 a{ vertices[indices[i+0]].position };
    vec3 b{ vertices[indices[i+1]].position };
    vec3 c{ vertices[indices[i+2]].position };

    vec3 edge1 = b - a;
    vec3 edge2 = c - a;
    vec3 rayCrossE2 = rayDirection.Cross(edge2);
    float det = edge1.Dot(rayCrossE2);

    if (det > -epsilon && det < epsilon)
      continue;

    float invDet = 1.f / det;
    vec3 s = rayOrigin - a;
    float u = invDet * s.Dot(rayCrossE2);

    if (u < 0 || u > 1)
      continue;

    vec3 sCrossE1 = s.Cross(edge1);
    float v = invDet * rayDirection.Dot(sCrossE1);

    if (v < 0 || u + v > 1)
      continue;

    float dist = invDet * edge2.Dot(sCrossE1);

    if (dist > epsilon && dist < result.distance) {
      result.distance = dist;
      if constexpr (bUseMeshNormals)
        result.normal = vertices[indices[i]].normal;
      else
        result.normal = edge2.Cross(edge1);
      result.bHit = true;
    }
  }

  result.position = ray.origin + ray.direction * result.distance;
  result.normal = mathf::normalize(meshTransform.transformDirection(result.normal));
  return result;
}

}
