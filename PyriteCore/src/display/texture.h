#pragma once

#include <string>
#include <array>
#include <vector>

struct ID3D11Resource;
struct ID3D11ShaderResourceView;
struct ID3D11DepthStencilView;
struct ID3D11SamplerState;

namespace pyr
{
struct GlobalTextureSet ;
class GraphicalResourceRegistry;


struct Texture
{

public:

  Texture() : m_resource(nullptr), m_texture(nullptr), m_width(0), m_height(0) {}

  ID3D11ShaderResourceView *getRawTexture() const { return m_texture; }
  ID3D11Resource *getRawResource() const { return m_resource; }
  ID3D11DepthStencilView* toDepthStencilView();
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

  Texture(float* data, size_t width, size_t height, bool bStagingTexture = false);

private:
  friend class TextureManager;
  friend class GraphicalResourceRegistry;
  friend class FrameBuffer;
  Texture(ID3D11Resource *resource, ID3D11ShaderResourceView *raw, size_t width, size_t height)
    : m_resource(resource), m_texture(raw), m_width(width), m_height(height) { }


  size_t m_width, m_height;
  ID3D11Resource *m_resource;
  ID3D11ShaderResourceView *m_texture;
  ID3D11DepthStencilView *m_asDepthView = nullptr;
};

struct GlobalTextureSet {
    Texture WhitePixel;
    Texture BlackPixel;
};

struct Cubemap
{
  Cubemap() : m_resource(nullptr), m_texture(nullptr) {}
  Cubemap(ID3D11Resource *resource, ID3D11ShaderResourceView *srv) : m_resource(resource), m_texture(srv) {} 

  ID3D11ShaderResourceView *getRawCubemap() const { return m_texture; }
  ID3D11Resource            *getRawResource() const { return m_resource; }
  void releaseRawCubemap();

private:
  friend class TextureManager;
  friend class GraphicalResourceRegistry;

  ID3D11Resource *m_resource;
  ID3D11ShaderResourceView *m_texture;
};

struct TextureArray
{

    size_t m_width, m_height;
    ID3D11Resource* m_resource;
    ID3D11ShaderResourceView* m_textureArray;
    std::vector<Texture> m_textures;
    size_t m_arrayCount;
    bool m_isCubeArray = false;

public:

    TextureArray(size_t width, size_t height, size_t count = 8, bool bIsDepthOnly = false, bool bIsCubeArray = false);
    ~TextureArray();
    static void CopyToTextureArray(const std::vector<Texture>& textures, TextureArray& outArray);
    static void CopyToTextureArray(const std::vector<Cubemap>& cubes, TextureArray& outArray);
    
    ID3D11ShaderResourceView* getRawTexture()   const { return m_textureArray; }
    ID3D11Resource* getRawResource()            const { return m_resource; }
    size_t getWidth()                           const { return m_width; }
    size_t getHeight()                          const { return m_height; }
    size_t getArrayCount()                      const { return m_arrayCount; }
    bool isCubeArray()                          const { return m_isCubeArray; }

};
//===============================================================================================================================//

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

  static Texture loadTexture(const std::wstring &path, bool bGenerateMips = true);
  static Cubemap loadCubemap(const std::wstring &path);
  static const SamplerState &getSampler(SamplerState::SamplerType type);

private:
  static std::array<SamplerState, SamplerState::SamplerType::_COUNT> s_samplers;

};




}
