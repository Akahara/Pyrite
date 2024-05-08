#pragma once

#include <memory>

#include "Texture.h"
#include "Shader.h"
#include "InputLayout.h"

namespace pyr
{

/*
 * The primary way to load and store graphical resources, see the comment in graphical_managed_resource
 */
class GraphicalResourceRegistry
{
private:
  using filepath = std::wstring;
  template<class T, class K>
  using map = std::unordered_map<T, K>;
  template<class T>
  using shared_ptr = std::shared_ptr<T>;

public:
  GraphicalResourceRegistry() = default;
  ~GraphicalResourceRegistry();

  GraphicalResourceRegistry(const GraphicalResourceRegistry &) = delete;
  GraphicalResourceRegistry &operator=(const GraphicalResourceRegistry &) = delete;
  GraphicalResourceRegistry(GraphicalResourceRegistry &&) = delete;
  GraphicalResourceRegistry &operator=(GraphicalResourceRegistry &&) = delete;

  Texture loadTexture(const filepath &path);
  Cubemap loadCubemap(const filepath &path);
  Effect *loadEffect(const filepath &path, const InputLayout& layout);

  void reloadShaders();

private:
  map<filepath, Texture> m_texturesCache;
  map<filepath, Cubemap> m_cubemapsCache;
  map<filepath, std::pair<Effect, InputLayout>> m_effects;
};

}