#pragma once

#include <vector>

#include "Shader.h"
#include "Texture.h"
#include "engine/Device.h"

namespace pyr {

class FrameBuffer
{
public:
  using target_t = uint8_t;
  enum Target : target_t {
    COLOR_0       = 1 << 0,
    COLOR_1       = 1 << 1,
    COLOR_2       = 1 << 2,
    COLOR_3       = 1 << 3,
    DEPTH_STENCIL = 1 << 4,
    __COUNT_COLOR = 4,
    __COUNT       = 5,
    MULTISAMPLED  = 1 << 5,
  };

  FrameBuffer() : m_width(0), m_height(0) {}
  ~FrameBuffer();

  // standard constructor
  FrameBuffer(unsigned int width, unsigned int height, target_t targets);
  FrameBuffer(target_t targets) : FrameBuffer(pyr::Device::getWinWidth(), pyr::Device::getWinHeight(), targets)
  {}
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
  void bindToD3DContext() const;
  void unbind();
  void clearTargets() const;
  void setDepthOverride(ID3D11DepthStencilView* depth);
  Texture getTargetAsTexture(Target target) const;
  // Helper for classes that define another enum for clarity
  Texture getTargetAsTexture(uint8_t target) const { return getTargetAsTexture(static_cast<Target>(target)); }
  unsigned int GetColoredTargetCount() const;

  static size_t targetTypeToIndex(Target target);
private:

  static std::vector<FrameBuffer *> s_frameBuffersStack;
  void CreateColorTarget(Target colorTarget, bool bIsMultisampled = false);

  unsigned int m_width, m_height;
  bool m_keepTextures = false;
  std::vector<ID3D11Texture2D*> m_textures;
  std::array<Texture, Target::__COUNT> m_targetsAsTextures;
  //ID3D11RenderTargetView *m_renderTargetView = nullptr;
  ID3D11DepthStencilView *m_depthStencilView = nullptr;
  ID3D11DepthStencilView *m_overridenDepth = nullptr;

  std::array<ID3D11RenderTargetView*, Target::__COUNT_COLOR> m_renderTargetViews;
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

  static InputLayout getBlitVertexLayout();
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


// Not sure how to call this, but this is basically a gpu side computed cubemap
struct CubemapFramebuffer
{
public:

    enum Face : uint8_t
    {
        X_PLUS,
        X_MINUS,
        Y_PLUS,
        Y_MINUS,
        Z_PLUS,
        Z_MINUS
    };

    CubemapFramebuffer() = default;
    CubemapFramebuffer(const CubemapFramebuffer&) = delete;
    CubemapFramebuffer& operator=(const CubemapFramebuffer&) = delete;
    CubemapFramebuffer(CubemapFramebuffer&& moved) noexcept
        : m_depths (std::exchange(moved.m_depths, {}))
        , m_resolution (std::exchange(moved.m_resolution, {}))
        , m_rtvs (std::exchange(moved.m_rtvs, {}))
        , m_textures (std::exchange(moved.m_textures, {}))
        , m_targetsAsCubemaps (std::exchange(moved.m_targetsAsCubemaps, {}))
    {}

    CubemapFramebuffer& operator=(CubemapFramebuffer&& moved) noexcept {
        CubemapFramebuffer{ std::move(moved) }.swap(*this);
        return *this;
    }
    void swap(CubemapFramebuffer& other) noexcept
    {
        std::swap(m_depths, other.m_depths);
        std::swap(m_resolution, other.m_resolution);
        std::swap(m_rtvs, other.m_rtvs);
        std::swap(m_textures, other.m_textures);
        std::swap(m_targetsAsCubemaps, other.m_targetsAsCubemaps);
    }
    ~CubemapFramebuffer();

    CubemapFramebuffer(size_t resolution, FrameBuffer::target_t targets)
        : m_resolution(std::max<UINT>(1U, static_cast<UINT>(resolution)))
    {
        // If we have a color
        if (targets & FrameBuffer::Target::COLOR_0) {
            D3D11_TEXTURE2D_DESC textureDesc = {};
            textureDesc.Width = m_resolution;
            textureDesc.Height = m_resolution;
            textureDesc.MipLevels = 1; // Set to the number of mip levels you want
            textureDesc.ArraySize = 6;         // 6 faces for the cube
            textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // Texture format
            textureDesc.SampleDesc.Count = 1;
            textureDesc.Usage = D3D11_USAGE_DEFAULT;
            textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
            textureDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE; // Flag for a cubemap

            ID3D11Texture2D* resource;
            DXTry(Engine::d3ddevice().CreateTexture2D(&textureDesc, nullptr, &resource), "Could not create a texture 2D");
            m_textures.push_back(resource);

            for (uint8_t face = 0; face < 6; face++)
            {
                D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
                rtvDesc.Format = textureDesc.Format;
                rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                rtvDesc.Texture2DArray.MipSlice = 0;
                rtvDesc.Texture2DArray.FirstArraySlice = face;
                rtvDesc.Texture2DArray.ArraySize = 1;

                DXTry(Engine::d3ddevice().CreateRenderTargetView(resource, &rtvDesc, &m_rtvs[face]), "Could not create the rtv array of a cubemap");
            }

            ID3D11ShaderResourceView* cubeSRV;
            D3D11_SHADER_RESOURCE_VIEW_DESC srDesc;
            srDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            srDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
            srDesc.Texture2D.MipLevels = 1;
            srDesc.Texture2D.MostDetailedMip = 0;
            srDesc.Texture2DArray.FirstArraySlice = 0;
            srDesc.Texture2DArray.ArraySize = 6;
            DXTry(Engine::d3ddevice().CreateShaderResourceView(resource, &srDesc, &cubeSRV), "Failed to create an SRV for the entire cube");
            m_targetsAsCubemaps[FrameBuffer::targetTypeToIndex(FrameBuffer::Target::COLOR_0)] = Cubemap(resource, cubeSRV);
        }


        if (targets & FrameBuffer::Target::DEPTH_STENCIL) {
            D3D11_TEXTURE2D_DESC depthTextureDesc;
            ZeroMemory(&depthTextureDesc, sizeof(depthTextureDesc));
            depthTextureDesc.Width = m_resolution;
            depthTextureDesc.Height = m_resolution;
            depthTextureDesc.MipLevels = 1;
            depthTextureDesc.ArraySize = 6;
            depthTextureDesc.Format = DXGI_FORMAT_R32_TYPELESS;
            depthTextureDesc.SampleDesc.Count =  1;
            depthTextureDesc.Usage = D3D11_USAGE_DEFAULT;
            depthTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
            depthTextureDesc.CPUAccessFlags = 0;
            depthTextureDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

            ID3D11Texture2D* depthTexture;
            DXTry(Engine::d3ddevice().CreateTexture2D(&depthTextureDesc, NULL, &depthTexture), "Could not create a depth texture for a framebuffer");
            m_textures.push_back(depthTexture);

            for (uint8_t face = 0; face < 6; face++)
            {
                D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
                ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
                depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
                depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                depthStencilViewDesc.Texture2DArray.MipSlice = 0;
                depthStencilViewDesc.Texture2DArray.FirstArraySlice = face;
                depthStencilViewDesc.Texture2DArray.ArraySize = 1;
                DXTry(Engine::d3ddevice().CreateDepthStencilView(depthTexture, &depthStencilViewDesc, &m_depths[face]), "Could not create a depth stencil view for a framebuffer");
            }

            D3D11_SHADER_RESOURCE_VIEW_DESC srDesc;
            srDesc.Format = DXGI_FORMAT_R32_FLOAT;
            srDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
            srDesc.TextureCube.MipLevels = static_cast<UINT>(1);
            srDesc.TextureCube.MostDetailedMip = 0;

            ID3D11ShaderResourceView* shaderResourceView;
            DXTry(Engine::d3ddevice().CreateShaderResourceView(depthTexture, &srDesc, &shaderResourceView), "Could not create a shader resource view for a framebuffer");
            m_targetsAsCubemaps[FrameBuffer::targetTypeToIndex(FrameBuffer::Target::DEPTH_STENCIL)] = Cubemap(depthTexture, shaderResourceView);
        }

    }
    // Tricky, this is hard to work with the framebuffer pipeline. Don't forget to get the pipeline and rebind the current FBO after doing this
    void bindFace(CubemapFramebuffer::Face face)
    {
        D3D11_VIEWPORT viewport{ 0,0,static_cast<float>(m_resolution),static_cast<float>(m_resolution),0,1 };
        Engine::d3dcontext().OMSetRenderTargets(1, &m_rtvs[face], m_depths[face]);
        Engine::d3dcontext().RSSetViewports(1, &viewport);
    }

    void clearFaceTargets(CubemapFramebuffer::Face face)
    {
        auto& context = Engine::d3dcontext();
        constexpr float clearColor[4]{ .05f, .08f, .1f, 0.0f };
        if (m_depths[face]) context.ClearDepthStencilView(m_depths[face], D3D11_CLEAR_DEPTH, 1.0f, 0);
        if (m_rtvs[face]) context.ClearRenderTargetView(m_rtvs[face], clearColor);
    }

    Cubemap getTargetAsCubemap(FrameBuffer::Target target) const;

private:
    UINT m_resolution;

    std::vector<ID3D11Texture2D*> m_textures;

    std::array<ID3D11RenderTargetView*, 6> m_rtvs{};
    std::array<ID3D11DepthStencilView*, 6> m_depths{};

    std::array<Cubemap, FrameBuffer::Target::__COUNT> m_targetsAsCubemaps;

};


}
