#include "graphical_resource.h"

#include "engine/directxlib.h"

namespace pyr
{

GraphicalResourceRegistry::~GraphicalResourceRegistry()
{
  for (auto &[_, texture] : m_texturesCache) {
    texture.m_resource->Release();
    texture.m_texture->Release();
  }
  for (auto &[_, cubemap] : m_cubemapsCache) {
    cubemap.m_resource->Release();
    cubemap.m_texture->Release();
  }
  for (auto &[_, effect] : m_effects) {
    DXRelease(effect.first.m_effect);
    DXRelease(effect.first.m_inputLayout);
  }
  // meshes are released on deletion
}

Texture GraphicalResourceRegistry::loadTexture(const filepath &path)
{
  if (m_texturesCache.contains(path))
    return m_texturesCache[path];
  return m_texturesCache[path] = TextureManager::loadTexture(path);
}

Cubemap GraphicalResourceRegistry::loadCubemap(const filepath &path)
{
  if (m_cubemapsCache.contains(path))
    return m_cubemapsCache[path];
  return m_cubemapsCache[path] = TextureManager::loadCubemap(path);
}

void GraphicalResourceRegistry::reloadShaders()
{
  for(auto &[file, oldEffect] : m_effects) {
    //try {
      Effect newEffect = ShaderManager::makeEffect(file, oldEffect.second);
      DXRelease(oldEffect.first.m_effect);
      oldEffect.first.m_constantBufferBindingsCache.clear();
      oldEffect.first.m_variableBindingsCache.clear();
      oldEffect.first.m_pass = newEffect.m_pass;
      oldEffect.first.m_effect = newEffect.m_effect;
      oldEffect.first.m_technique = newEffect.m_technique;
    //} catch (const std::runtime_error &e) {
      //logs::graphics.logm(utils::widestring2string(file), " compilation error: ", e.what());
    //}
  }
}

Effect *GraphicalResourceRegistry::loadEffect(const filepath &path, const ShaderVertexLayout &layout)
{
  if (m_effects.contains(path))
    return &m_effects[path].first;
  return &(m_effects[path] = { ShaderManager::makeEffect(path, layout), layout }).first;
}

}
