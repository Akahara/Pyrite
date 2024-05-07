#pragma once

#include <vector>

#include "shader.h"
#include "texture.h"
#include "engine/d3ddevice.h"

namespace pyr {

class FrameBuffer
{
public:
  using target_t = uint8_t;
  enum Target : target_t {
    COLOR_0       = 1 << 0,
    DEPTH_STENCIL = 1 << 1,
    __COUNT       = 2,
    MULTISAMPLED  = 1 << 2,
  };

  FrameBuffer() : m_width(0), m_height(0) {}
  ~FrameBuffer();

  // standard constructor
  FrameBuffer(unsigned int width, unsigned int height, target_t targets);
  // primary target constructor
  FrameBuffer(IDXGISwapChain *swapChain, ID3D11Device *device, unsigned int screenWidth, unsigned int screenHeight);

  FrameBuffer(const FrameBuffer &) = delete;
  FrameBuffer &operator=(const FrameBuffer &) = delete;
  FrameBuffer(FrameBuffer &&moved) noexcept;
  FrameBuffer &operator=(FrameBuffer &&moved) noexcept {
    FrameBuffer{ std::move(moved) }.swap(*this);
    return *this;
  }

  static FrameBuffer &getActiveFrameBuffer() { return *s_frameBuffersStack.back(); }

  void swap(FrameBuffer &other, bool checkForBoundBuffers=true) noexcept;

  void bind();
  void unbind();
  void clearTargets() const;
  Texture getTargetAsTexture(Target target) const;

private:
  void bindToD3DContext() const;
  static size_t targetTypeToIndex(Target target);

  static std::vector<FrameBuffer *> s_frameBuffersStack;

  unsigned int m_width, m_height;
  bool m_keepTextures = false;
  std::vector<ID3D11Texture2D*> m_textures;
  std::array<Texture, Target::__COUNT> m_targetsAsTextures;
  ID3D11RenderTargetView *m_renderTargetView = nullptr;
  ID3D11DepthStencilView *m_depthStencilView = nullptr;
};

class FrameBufferPipeline {
public:
  struct fullscreen_t {};

  FrameBufferPipeline(unsigned int width, unsigned int height);
  explicit FrameBufferPipeline(fullscreen_t);

  // call before drawing any geometry
  void bindGeometry();
  // call after all geometry draws have been made
  void endGeometry();
  // call before every vfx pass, returns the previous render target texture
  // (the geometry pass result for the first call, the previous vfx result
  // for subsequent calls)
  Texture swap();
  // returns the latest geometry pass' depth texture
  Texture getDepthTexture() const;
  // call after all vfx passes, returns the final image
  Texture endFrame();

  // returns a frame rendered by a previous call to endFrame()
  // if pastFrameCount is 0 the previous frame is returned
  // pastFrameCount must be less than SAVED_FRAMES-1
  Texture getPreviousFrame(uint8_t pastFrameCount) const;

  static ShaderVertexLayout getBlitVertexLayout();
  static void doBlitDrawCall();
  static void doSimpleBlit(const Texture &blitTexture);
  static void doMSAASimpleBlit(const Texture &multisampledBlitTexture);

  static void loadGlobalResources(GraphicalResourceRegistry &resources);
  static void unloadGlobalResources();

private:
  static constexpr size_t SAVED_FRAMES = 2;

  std::shared_ptr<ScreenResizeEventHandler> m_windowResizeEventHandler; // for fullscreen pipelines

  uint8_t m_currentGeometryIndex = 0;
  uint8_t m_currentPingPongIndex = 0;
  FrameBuffer m_geometryBuffer[SAVED_FRAMES];
  FrameBuffer m_pingPongBuffers[2];
  FrameBuffer *m_activeBuffer;
};

}
