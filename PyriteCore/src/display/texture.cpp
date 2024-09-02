#include "Texture.h"

#include <stdexcept>

#include "ddstextureloader/WICTextureLoader11.h"
#include "ddstextureloader/DDSTextureLoader11.h"
#include "display/GraphicalResource.h"
#include "engine/Directxlib.h"
#include "engine/Engine.h"
#include "utils/StringUtils.h"
#include <filesystem>
#include <stbi/stb_image.h>

namespace pyr
{

GlobalTextureSet s_globalTextureSet;

std::array<SamplerState, SamplerState::SamplerType::_COUNT> TextureManager::s_samplers;

Texture TextureManager::loadTexture(const std::wstring &path)
{
  auto &device = Engine::d3ddevice();
  ID3D11Resource *resource;
  ID3D11ShaderResourceView *texture;
  ID3D11Texture2D *textureInterface;
  std::filesystem::path fspath = path;
  auto extension = fspath.extension();

  if (extension == ".dds")
  {
	  if (auto hr = DirectX::CreateDDSTextureFromFile(
		  &device,
		  &Engine::d3dcontext(),
		  fspath.c_str(),
		  &resource,
		  &texture); hr!= S_OK)
		throw std::runtime_error("Could not load texture " + pyr::widestring2string(path));
  }
  else if (extension == ".hdr")
  {
	  int width, height, channels;
	  float* data = stbi_loadf(widestring2string(path.c_str()).c_str(), &width, &height, &channels, 4);
	  Texture outTexture{ data, (size_t)width, (size_t)height };
	  stbi_image_free(data);
	  return outTexture;
  }
  else
  {
	if (DirectX::CreateWICTextureFromFile(
		  &device,
		  &Engine::d3dcontext(),
		  fspath.c_str(),
		  &resource,
		  &texture
	) != S_OK)
		throw std::runtime_error("Could not load texture " + pyr::widestring2string(path));
  }

  resource->QueryInterface<ID3D11Texture2D>(&textureInterface);
  D3D11_TEXTURE2D_DESC desc;
  textureInterface->GetDesc(&desc);
  DXRelease(textureInterface);

  return Texture(resource, texture, desc.Width, desc.Height);
}

Cubemap TextureManager::loadCubemap(const std::wstring &path)
{
  auto &device = Engine::d3ddevice();
  auto &context = Engine::d3dcontext();

  ID3D11Resource *resource;
  ID3D11ShaderResourceView *texture;

  // TODO : also import png here

  if (DirectX::CreateDDSTextureFromFileEx(
	  &device,
	  &context,
	  path.c_str(),
	  0,
	  D3D11_USAGE_DEFAULT,
	  D3D11_BIND_SHADER_RESOURCE,
	  D3D11_CPU_ACCESS_READ,
	  D3D11_RESOURCE_MISC_TEXTURECUBE,
	  DirectX::DDS_LOADER_DEFAULT,
	  &resource,
	  &texture
	) != S_OK)
	throw std::runtime_error("Could not load texture " + pyr::widestring2string(path));

  return Cubemap(resource, texture);
}

ID3D11DepthStencilView* Texture::toDepthStencilView()
{
	if (!m_asDepthView)
	{
		auto& device = pyr::Engine::d3ddevice();
		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
		ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
		depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Texture2D.MipSlice = 0;
		device.CreateDepthStencilView(m_resource, &depthStencilViewDesc, &m_asDepthView);
	}

	return m_asDepthView;
}

void Texture::releaseRawTexture()
{
  DXRelease(m_resource);
  DXRelease(m_texture);
  m_width = m_height = 0;
}

void Texture::initDefaultTextureSet(GraphicalResourceRegistry& registry)
{
	float whitePixel_32f4[] = {1,1,1,1};
	float blackPixel_32f4[] = {1,1,1,1};
	Texture textureWhitePixel{ whitePixel_32f4, 1,1 };
	Texture textureBlackPixel{ blackPixel_32f4, 1,1 };
	registry.keepHandleToTexture(textureWhitePixel);
	registry.keepHandleToTexture(textureBlackPixel);
	s_globalTextureSet.WhitePixel = textureWhitePixel;
	s_globalTextureSet.BlackPixel = textureBlackPixel;
}

const GlobalTextureSet& Texture::getDefaultTextureSet()
{
	return s_globalTextureSet;
}

// This is intended to be used with .hdr files
Texture::Texture(float* data, size_t width, size_t height, bool bStagingTexture /* = false */) : m_width(width), m_height(height)
{

	D3D11_TEXTURE2D_DESC srcDesc;
	ZeroMemory(&srcDesc, sizeof(srcDesc));

	srcDesc.Width = static_cast<UINT>(width);
	srcDesc.Height = static_cast<UINT>(height);
	srcDesc.MipLevels = 1;
	srcDesc.ArraySize = 1;
	srcDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	srcDesc.SampleDesc.Count = 1;
	srcDesc.SampleDesc.Quality = 0;
	srcDesc.Usage = bStagingTexture ? D3D11_USAGE_STAGING : D3D11_USAGE_DEFAULT;
	srcDesc.BindFlags = bStagingTexture ? 0 : D3D11_BIND_SHADER_RESOURCE;
	srcDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	srcDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA initialData{};
	initialData.pSysMem = data;
	initialData.SysMemPitch = static_cast<UINT>(width) * 4 * sizeof(float);
	initialData.SysMemSlicePitch = 0; // 0 is for tex2D, create SRV fails otherwise...

	ID3D11Texture2D* resource;

	DXTry(
		Engine::d3ddevice().CreateTexture2D(
			&srcDesc,
			&initialData, &resource),
		"Could not create a texture 2D");

	m_resource = resource;
	
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = srcDesc.Format;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = srcDesc.MipLevels;

	auto hr = Engine::d3ddevice().CreateShaderResourceView(
		m_resource ,
		&srvDesc, &m_texture);
}

TextureManager::~TextureManager()
{
  for (SamplerState &sampler : s_samplers)
	if(sampler.m_samplerState)
	  sampler.m_samplerState->Release();
}

const SamplerState &TextureManager::getSampler(SamplerState::SamplerType type)
{
  if (s_samplers[type].m_samplerState == nullptr) {
	D3D11_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 4;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	DXTry(
	  Engine::d3ddevice().CreateSamplerState(
		&samplerDesc,
		&s_samplers[type].m_samplerState),
	  "Could not create a texture sampler state");
  }
  return s_samplers[type];
}

}
