#include "GraphicalResource.h"

#include "engine/Directxlib.h"
#include "utils/Logger.h"
#include "utils/StringUtils.h"

#include <utility>
#include <algorithm>

namespace pyr
{

PYR_DEFINELOG(GraphicalResourceRegistry, INFO);

GraphicalResourceRegistry::GraphicalResourceRegistry(GraphicalResourceRegistry&& other) noexcept
    : m_cubemapsCache{ std::exchange(other.m_cubemapsCache, {}) }
    , m_texturesCache{ std::exchange(other.m_texturesCache, {}) }
    , m_effects{ std::exchange(other.m_effects, {}) }

{}

GraphicalResourceRegistry& GraphicalResourceRegistry::operator=(GraphicalResourceRegistry&& other) noexcept
{
    GraphicalResourceRegistry{ std::move(other) }.swap(*this);
    return *this;
}


void GraphicalResourceRegistry::swap(GraphicalResourceRegistry& other) noexcept
{
    std::swap(m_texturesCache, other.m_texturesCache);
    std::swap(m_effects, other.m_effects);
    std::swap(m_cubemapsCache, other.m_cubemapsCache);
}


GraphicalResourceRegistry::~GraphicalResourceRegistry()
{
  for (auto &[_, texture] : m_texturesCache) {
    texture.m_resource->Release();
    texture.m_texture->Release();
  }
  for (auto& texture : m_ownedTextures) {
      //texture.m_resource->Release();
      //texture.m_texture->Release(); // < causes a memory leak, todo fix
  }
  for (auto &[_, cubemap] : m_cubemapsCache) {
    cubemap.m_resource->Release();
    cubemap.m_texture->Release();
  }

  // meshes are released on deletion
}

Texture GraphicalResourceRegistry::loadTexture(const filepath &path)
{
  if (m_texturesCache.contains(path))
    return m_texturesCache[path];
  return m_texturesCache[path] = TextureManager::loadTexture(path);
}

void GraphicalResourceRegistry::keepHandleToTexture(Texture texture)
{
    m_ownedTextures.push_back(texture);
}

void GraphicalResourceRegistry::keepHandleToCubemap(Cubemap cubemap)
{
    m_ownedCubemaps.push_back(cubemap);
}

Cubemap GraphicalResourceRegistry::loadCubemap(const filepath &path)
{
  if (m_cubemapsCache.contains(path))
    return m_cubemapsCache[path];
  return m_cubemapsCache[path] = TextureManager::loadCubemap(path);
}

Effect *GraphicalResourceRegistry::loadEffect(const filepath &path, const InputLayout &layout)
{
  if (m_effects.contains(path))
    return &*m_effects[path].first;
  return &*(m_effects[path] = { ShaderManager::makeEffect(path, layout), layout }).first;
}

}
