#pragma once

#include <string>
#include <array>

struct ID3D11Resource;
struct ID3D11ShaderResourceView;
struct ID3D11SamplerState;

namespace pyr
{
struct GlobalTextureSet ;
class GraphicalResourceRegistry;


struct Texture
{

  Texture() : m_resource(nullptr), m_texture(nullptr), m_width(0), m_height(0) {}

  ID3D11ShaderResourceView *getRawTexture() const { return m_texture; }
  ID3D11Resource *getRawResource() const { return m_resource; }
  size_t getWidth() const { return m_width; }
  size_t getHeight() const { return m_height; }

  // releases the underlying texture object, must only be called by the resource manager (see managed_resource)
  void releaseRawTexture();

  // returns whether this object holds a valid texture handle, default-constructed textures do not.
  // note that this method doesn't say anything about texture validity, the handle may have been
  // destroyed by the GraphicalResourceRegistry.
  bool empty() const { return m_texture == nullptr; }

  bool operator==(const Texture &other) const { return m_texture == other.m_texture; }
  bool operator!=(const Texture &other) const { return m_texture != other.m_texture; }

  static void initDefaultTextureSet(GraphicalResourceRegistry& registry);
  static const GlobalTextureSet& getDefaultTextureSet();

private:
  friend class TextureManager;
  friend class GraphicalResourceRegistry;
  friend class FrameBuffer;
  Texture(ID3D11Resource *resource, ID3D11ShaderResourceView *raw, size_t width, size_t height)
    : m_resource(resource), m_texture(raw), m_width(width), m_height(height) { }

  Texture(float* data, size_t width, size_t height);

  size_t m_width, m_height;
  ID3D11Resource *m_resource;
  ID3D11ShaderResourceView *m_texture;
};

struct GlobalTextureSet {
    Texture WhitePixel;
    Texture BlackPixel;
};

struct Cubemap
{
  Cubemap() : m_resource(nullptr), m_texture(nullptr) {}
  Cubemap(ID3D11Resource *resource, ID3D11ShaderResourceView *raw) : m_resource(resource), m_texture(raw) {} // dont ùake this public

  ID3D11ShaderResourceView *getRawCubemap() const { return m_texture; }

private:
  friend class TextureManager;
  friend class GraphicalResourceRegistry;

  ID3D11Resource *m_resource;
  ID3D11ShaderResourceView *m_texture;
};

struct SamplerState
{
public:
  enum SamplerType
  {
    BASIC,
    _COUNT
  };

  SamplerState() : m_samplerState(nullptr) {}

  ID3D11SamplerState *getRawSampler() const { return m_samplerState; }

private:
  friend class TextureManager;
  explicit SamplerState(ID3D11SamplerState *state) : m_samplerState(state) {}

  ID3D11SamplerState *m_samplerState;
};

class TextureManager
{
public:
  ~TextureManager();

  static Texture loadTexture(const std::wstring &path);
  static Cubemap loadCubemap(const std::wstring &path);
  static const SamplerState &getSampler(SamplerState::SamplerType type);

private:
  static std::array<SamplerState, SamplerState::SamplerType::_COUNT> s_samplers;

};




}
