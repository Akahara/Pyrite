#pragma once

#include "Mesh/StaticMesh.h"

namespace pyr
{

using ActorId = std::string;
  
class Actor
{
public:
  virtual void tick(float deltaTime) {}
  virtual ~Actor() = default;

  void setMesh(std::unique_ptr<StaticMesh>&& mesh) { m_mesh = std::move(mesh); }
  [[nodiscard]] const StaticMesh* getMesh() const { return m_mesh.get(); }
  [[nodiscard]] const Transform& getTransform() const { return m_transform; }
  [[nodiscard]] Transform& getTransform() { return m_transform; }

  const ActorId& getId() const { return m_id; }
  void setId(ActorId id) { m_id = std::move(id); }

private:
  std::unique_ptr<StaticMesh> m_mesh;
  Transform m_transform;
  ActorId m_id;
};

}
