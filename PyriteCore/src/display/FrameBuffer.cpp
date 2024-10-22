#include "FrameBuffer.h"

#include <algorithm>

#include "GraphicalResource.h"
#include "RenderProfiles.h"
#include "Shader.h"
#include "engine/Engine.h"
#include "utils/Debug.h"
#include "utils/Math.h"
#include "InputLayout.h"

namespace pyr
{

static constexpr unsigned int MSAA_LEVEL = 8;
static_assert(MSAA_LEVEL == 8); // the number of samples must set as a constant in msaa.hlsl

std::vector<FrameBuffer *> FrameBuffer::s_frameBuffersStack;

FrameBuffer::FrameBuffer(unsigned int width, unsigned int height, target_t targets)
  : m_width(std::max<UINT>(1, width))
  , m_height(std::max<UINT>(1, height))
{
  auto &device = Engine::d3ddevice();

  bool isMultisampled = targets & Target::MULTISAMPLED;

  if (targets & Target::COLOR_0) {
	ID3D11Texture2D *renderTexture;
	CD3D11_TEXTURE2D_DESC renderTextureDesc;
	ZeroMemory(&renderTextureDesc, sizeof(renderTextureDesc));
	renderTextureDesc.Width =  m_width;
	renderTextureDesc.Height = m_height;
	renderTextureDesc.MipLevels = 1;
	renderTextureDesc.ArraySize = 1;
	renderTextureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	renderTextureDesc.SampleDesc.Count = isMultisampled ? MSAA_LEVEL : 1;
	renderTextureDesc.SampleDesc.Quality = 0;
	renderTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	renderTextureDesc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
	DXTry(device.CreateTexture2D(&renderTextureDesc, NULL, &renderTexture), "Could not create a texture for a framebuffer");
	m_textures.push_back(renderTexture);

	D3D11_RENDER_TARGET_VIEW_DESC rtDesc;
	rtDesc.Format = renderTextureDesc.Format;
	rtDesc.ViewDimension = isMultisampled ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;
	rtDesc.Texture2D.MipSlice = 0;
	DXTry(device.CreateRenderTargetView(renderTexture, &rtDesc, &m_renderTargetView), "Could not create a render target for a framebuffer");

	D3D11_SHADER_RESOURCE_VIEW_DESC srDesc;
	srDesc.Format = renderTextureDesc.Format;
	srDesc.ViewDimension = isMultisampled ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
	srDesc.Texture2D.MostDetailedMip = 0;
	srDesc.Texture2D.MipLevels = 1;
	ID3D11ShaderResourceView *shaderResourceView;
	DXTry(device.CreateShaderResourceView(renderTexture, &srDesc, &shaderResourceView), "Could not create a shader resource view for a framebuffer");
	m_targetsAsTextures[targetTypeToIndex(Target::COLOR_0)] = Texture(renderTexture, shaderResourceView, m_width, m_height);
  }

  if (targets & Target::DEPTH_STENCIL) {
	D3D11_TEXTURE2D_DESC depthTextureDesc;
	ZeroMemory(&depthTextureDesc, sizeof(depthTextureDesc));
	depthTextureDesc.Width =  m_width;
	depthTextureDesc.Height = m_height;
	depthTextureDesc.MipLevels = 1;
	depthTextureDesc.ArraySize = 1;
	depthTextureDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	depthTextureDesc.SampleDesc.Count = isMultisampled ? MSAA_LEVEL : 1;
	depthTextureDesc.SampleDesc.Quality = 0;
	depthTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	depthTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	depthTextureDesc.CPUAccessFlags = 0;
	depthTextureDesc.MiscFlags = 0;
	ID3D11Texture2D *depthTexture;
	DXTry(device.CreateTexture2D(&depthTextureDesc, NULL, &depthTexture), "Could not create a depth texture for a framebuffer");
	m_textures.push_back(depthTexture);

	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
	depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilViewDesc.ViewDimension = isMultisampled ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;
	DXTry(device.CreateDepthStencilView(depthTexture, &depthStencilViewDesc, &m_depthStencilView), "Could not create a depth stencil view for a framebuffer");

	D3D11_SHADER_RESOURCE_VIEW_DESC srDesc;
	srDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srDesc.ViewDimension = isMultisampled ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
	srDesc.Texture2D.MostDetailedMip = 0;
	srDesc.Texture2D.MipLevels = 1;
	ID3D11ShaderResourceView *shaderResourceView;
	DXTry(device.CreateShaderResourceView(depthTexture, &srDesc, &shaderResourceView), "Could not create a shader resource view for a framebuffer");
	m_targetsAsTextures[targetTypeToIndex(Target::DEPTH_STENCIL)] = Texture(depthTexture, shaderResourceView, m_width, m_height);
  }
}

FrameBuffer::FrameBuffer(IDXGISwapChain *swapChain, ID3D11Device *device, unsigned int screenWidth, unsigned int screenHeight)
  : m_width(screenWidth)
  , m_height(screenHeight)
  , m_keepTextures(true)
{
  ID3D11Texture2D *backBuffer;
  DXTry(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer), "Could not retrieve the screen backbuffer");
  DXTry(device->CreateRenderTargetView(backBuffer, NULL, &m_renderTargetView), "Could not create a render target view for the screen backbuffer");
  DXRelease(backBuffer);
}

void FrameBuffer::clearTargets() const
{
  auto &context = Engine::d3dcontext();
  constexpr float clearColor[4]{ .05f, .08f, .1f, 0.0f };
  if(m_depthStencilView) context.ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
  if(m_renderTargetView) context.ClearRenderTargetView(m_renderTargetView, clearColor);
}

Texture FrameBuffer::getTargetAsTexture(Target target) const
{
  Texture tex = m_targetsAsTextures[targetTypeToIndex(target)];
  PYR_ASSERT(!tex.empty(), "Tried to retrieve a non-existing texture");
  return tex;
}

FrameBuffer::~FrameBuffer()
{
#ifdef PYR_DEBUG
  if (std::ranges::find(s_frameBuffersStack, this) != s_frameBuffersStack.end())
  {
    //"Cannot delete a bound framebuffer!"; // TODO log
    return;
  }
#endif

  DXRelease(m_depthStencilView);
  DXRelease(m_renderTargetView);
  DXRelease(m_overridenDepth);
  if(!m_keepTextures) {
	  std::ranges::for_each(m_targetsAsTextures, [](auto texture) { texture.releaseRawTexture(); });
  }
}

FrameBuffer::FrameBuffer(FrameBuffer&& moved) noexcept
{
  m_width             = std::exchange(moved.m_width, {});
  m_height            = std::exchange(moved.m_height, {});
  m_depthStencilView  = std::exchange(moved.m_depthStencilView, {});
  m_renderTargetView  = std::exchange(moved.m_renderTargetView, {});
  m_textures          = std::exchange(moved.m_textures, {});
  m_targetsAsTextures = std::exchange(moved.m_targetsAsTextures, {});
  m_keepTextures      = std::exchange(moved.m_keepTextures, false);
}

void FrameBuffer::swap(FrameBuffer& other, bool checkForBoundBuffers) noexcept
{
  if(checkForBoundBuffers) {
	std::ranges::for_each(s_frameBuffersStack, [&](auto &activeBuffer) {
	  if (activeBuffer == this)
		activeBuffer = &other;
	  else if (activeBuffer == &other)
		activeBuffer = this;
	});
  }
  std::swap(m_width, other.m_width);
  std::swap(m_height, other.m_height);
  std::swap(m_depthStencilView, other.m_depthStencilView);
  std::swap(m_renderTargetView, other.m_renderTargetView);
  std::swap(m_textures, other.m_textures);
  std::swap(m_targetsAsTextures, other.m_targetsAsTextures);
  std::swap(m_keepTextures, other.m_keepTextures);
}

void FrameBuffer::bind()
{
  bindToD3DContext();
  s_frameBuffersStack.push_back(this);
  PYR_ASSERT(s_frameBuffersStack.size() < 10, "Too many framebuffers on the stack! Did you forget to unbind() some ?");
}

void FrameBuffer::unbind()
{
  if (m_renderTargetView == nullptr && m_depthStencilView == nullptr) return;
  PYR_ASSERT(!s_frameBuffersStack.empty() && s_frameBuffersStack.back() == this, "Tried to unbind a framebuffer that was not bound!");
  s_frameBuffersStack.pop_back();
  if (!s_frameBuffersStack.empty())
    s_frameBuffersStack.back()->bindToD3DContext();
}

void FrameBuffer::bindToD3DContext() const
{
  D3D11_VIEWPORT viewport{ 0,0,static_cast<float>(m_width),static_cast<float>(m_height),0,1 };
  Engine::d3dcontext().OMSetRenderTargets(1, &m_renderTargetView, m_overridenDepth ? m_overridenDepth : m_depthStencilView);
  Engine::d3dcontext().RSSetViewports(1, &viewport);
}

size_t FrameBuffer::targetTypeToIndex(Target target)
{
  return mathf::firstBitIndex(static_cast<target_t>(target));
}

void FrameBuffer::setDepthOverride(ID3D11DepthStencilView* depth)
{ 
	m_overridenDepth = depth; 
	bindToD3DContext();
}

struct GlobalResources {
  Effect *simpleBlitEffect;
  Effect *simpleMSAABlitEffect;
} *s_globalResources;

FrameBufferPipeline::FrameBufferPipeline(unsigned width, unsigned height)
  : m_activeBuffer(nullptr)
{
  for (size_t i = 0; i < SAVED_FRAMES; i++)
	m_geometryBuffer[i] = FrameBuffer(width, height, FrameBuffer::COLOR_0 | FrameBuffer::DEPTH_STENCIL | FrameBuffer::MULTISAMPLED);
  for (size_t i = 0; i < std::size(m_pingPongBuffers); i++)
	m_pingPongBuffers[i] = FrameBuffer(width, height, FrameBuffer::COLOR_0);
}

FrameBufferPipeline::FrameBufferPipeline(fullscreen_t)
  : FrameBufferPipeline(Device::getWinWidth(), Device::getWinHeight())
{
  Device::addWindowResizeEventHandler(m_windowResizeEventHandler = std::make_shared<ScreenResizeEventHandler>([this](int w, int h) {
	for (size_t i = 0; i < SAVED_FRAMES; i++)
	  m_geometryBuffer[i] = FrameBuffer(w, h, FrameBuffer::COLOR_0 | FrameBuffer::DEPTH_STENCIL | FrameBuffer::MULTISAMPLED);
	for (size_t i = 0; i < std::size(m_pingPongBuffers); i++)
	  m_pingPongBuffers[i] = FrameBuffer(w, h, FrameBuffer::COLOR_0);
  }));
}

void FrameBufferPipeline::bindGeometry()
{
  m_activeBuffer = &m_geometryBuffer[m_currentGeometryIndex];
  m_activeBuffer->bind();
  m_activeBuffer->clearTargets();
}

void FrameBufferPipeline::endGeometry()
{
  RenderProfiles::pushDepthProfile(DepthProfile::NO_DEPTH);
  RenderProfiles::pushBlendProfile(BlendProfile::NO_BLEND);

  Texture geometryFrame = m_activeBuffer->getTargetAsTexture(FrameBuffer::COLOR_0);
  m_activeBuffer->unbind();
  m_activeBuffer = &m_pingPongBuffers[m_currentPingPongIndex];
  m_activeBuffer->bind();
  doMSAASimpleBlit(geometryFrame);
}

Texture FrameBufferPipeline::swap()
{
  Effect::unbindResources();
  m_activeBuffer->unbind();
  m_currentPingPongIndex = 1-m_currentPingPongIndex;
  m_pingPongBuffers[m_currentPingPongIndex].bind();
  return std::exchange(m_activeBuffer, &m_pingPongBuffers[m_currentPingPongIndex])->getTargetAsTexture(FrameBuffer::COLOR_0);
}

Texture FrameBufferPipeline::getDepthTexture() const
{
  return m_geometryBuffer[m_currentGeometryIndex].getTargetAsTexture(FrameBuffer::DEPTH_STENCIL);
}

Texture FrameBufferPipeline::endFrame()
{
  RenderProfiles::popDepthProfile();
  RenderProfiles::popBlendProfile();
  Texture finalImage = m_activeBuffer->getTargetAsTexture(FrameBuffer::COLOR_0);
  m_activeBuffer->unbind();
  m_activeBuffer = nullptr;
  m_currentGeometryIndex = (m_currentGeometryIndex+1) % SAVED_FRAMES;
  return finalImage;
}

Texture FrameBufferPipeline::getPreviousFrame(uint8_t pastFrameCount) const
{
  PYR_ASSERT(pastFrameCount < SAVED_FRAMES-1, "Invalid pastFrameCount");
  uint8_t idx = 1+pastFrameCount > m_currentGeometryIndex
	? SAVED_FRAMES + m_currentGeometryIndex - 1 - pastFrameCount
	: m_currentGeometryIndex - 1 - pastFrameCount;
  return m_geometryBuffer[idx].getTargetAsTexture(FrameBuffer::COLOR_0);
}

InputLayout FrameBufferPipeline::getBlitVertexLayout()
{
  return {};
}

void FrameBufferPipeline::doBlitDrawCall()
{
  Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  Engine::d3dcontext().Draw(3, 0);
}

void FrameBufferPipeline::doSimpleBlit(const Texture &blitTexture)
{
  s_globalResources->simpleBlitEffect->bindTexture(blitTexture, "blitTexture");
  s_globalResources->simpleBlitEffect->bind();
  doBlitDrawCall();
}

void FrameBufferPipeline::doMSAASimpleBlit(const Texture &blitTexture)
{
  s_globalResources->simpleMSAABlitEffect->bindTexture(blitTexture, "msaaBlitTexture");
  s_globalResources->simpleMSAABlitEffect->bind();
  doBlitDrawCall();
}

void FrameBufferPipeline::loadGlobalResources(GraphicalResourceRegistry &resources)
{
  s_globalResources = new GlobalResources;
  s_globalResources->simpleBlitEffect = resources.loadEffect(L"res/shaders/blit_copy.fx", getBlitVertexLayout());
  s_globalResources->simpleMSAABlitEffect = resources.loadEffect(L"res/shaders/blit_mscopy.fx", getBlitVertexLayout());

}

void FrameBufferPipeline::unloadGlobalResources()
{
  delete s_globalResources;
}

Cubemap CubemapFramebuffer::getTargetAsCubemap(FrameBuffer::Target target) const
{
	Cubemap tex = m_targetsAsCubemaps[FrameBuffer::targetTypeToIndex(target)];
	return tex;
}

}
