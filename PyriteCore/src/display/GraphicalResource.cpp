#include "GraphicalResource.h"

#include "engine/Directxlib.h"
#include "utils/Logger.h"
#include "utils/StringUtils.h"

#include <utility>
#include <algorithm>

template<>
struct std::hash<pyr::Effect::define_t>
{
    std::size_t operator()(const pyr::Effect::define_t& s) const noexcept
    {
        std::size_t h1 = std::hash<std::string>{}(s.name);
        std::size_t h2 = std::hash<std::string>{}(s.value);
        return h1 ^ (h2 << 1);
    }
};

template<>
struct std::hash<std::vector<pyr::Effect::define_t>>
{
    std::size_t operator()(const std::vector<pyr::Effect::define_t>& vec) const noexcept
    {
        std::size_t seed = vec.size();
        for (const pyr::Effect::define_t& def : vec) {
            seed ^= std::hash<pyr::Effect::define_t>{}(def)+0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};


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

Texture GraphicalResourceRegistry::loadTexture(const filepath &path, bool bGenerateMips /* = true */)
{
  if (m_texturesCache.contains(path))
    return m_texturesCache[path];
  return m_texturesCache[path] = TextureManager::loadTexture(path, bGenerateMips);
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

Effect* GraphicalResourceRegistry::loadEffect(const filepath& path, const InputLayout& layout, const std::vector<Effect::define_t>& defines /* = {} */)
{
  size_t effectHash = std::hash<filepath>{}(path) ^ std::hash<std::vector<pyr::Effect::define_t>>{}(defines);
  if (m_effects.contains(effectHash))
    return &*m_effects[effectHash].first;
  return &*(m_effects[effectHash] = { ShaderManager::makeEffect(path, layout, defines), layout }).first;
}

}

