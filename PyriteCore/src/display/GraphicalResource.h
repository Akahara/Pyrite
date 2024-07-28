#pragma once

#include <memory>
#include <vector>

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
public:
  using filepath = std::wstring;
private:
  template<class T, class K>
  using map = std::unordered_map<T, K>;
  template<class T>
  using vector = std::vector<T>;

public:
  GraphicalResourceRegistry() = default;
  ~GraphicalResourceRegistry();

  GraphicalResourceRegistry(const GraphicalResourceRegistry &) = delete;
  GraphicalResourceRegistry &operator=(const GraphicalResourceRegistry &) = delete;
  GraphicalResourceRegistry(GraphicalResourceRegistry &&) noexcept;
  GraphicalResourceRegistry &operator=(GraphicalResourceRegistry &&) noexcept;

  Texture loadTexture(const filepath &path);
  void keepHandleToTexture(Texture texture);
  void keepHandleToCubemap(Cubemap cubemap);
  Cubemap loadCubemap(const filepath &path);
  Effect *loadEffect(const filepath &path, const InputLayout& layout);

  void swap(GraphicalResourceRegistry& other) noexcept;

private:
  map<filepath, Texture> m_texturesCache;
  map<filepath, Cubemap> m_cubemapsCache;
  vector<Texture> m_ownedTextures;
  vector<Cubemap> m_ownedCubemaps;
  map<filepath, std::pair<std::shared_ptr<Effect>, InputLayout>> m_effects;
};

}