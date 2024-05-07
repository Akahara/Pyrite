#include "texture.h"

#include <stdexcept>

#include "engine/directxlib.h"
#include "ddstextureloader/DDSTextureLoader11.h"

#include "engine/windowsengine.h"
#include "utils/stringutils.h"

namespace pyr
{

std::array<SamplerState, SamplerState::SamplerType::_COUNT> TextureManager::s_samplers;

Texture TextureManager::loadTexture(const std::wstring &path)
{
  auto &device = Engine::d3ddevice();
  ID3D11Resource *resource;
  ID3D11ShaderResourceView *texture;
  ID3D11Texture2D *textureInterface;
  if (DirectX::CreateDDSTextureFromFile(
	  &device,
	  &Engine::d3dcontext(),
	  path.c_str(),
	  &resource,
	  &texture) != S_OK)
	throw std::runtime_error("Could not load texture " + pyr::widestring2string(path));

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

  if (DirectX::CreateDDSTextureFromFileEx(
	  &device,
	  &context,
	  path.c_str(),
	  0,
	  D3D11_USAGE_DEFAULT,
	  D3D11_BIND_SHADER_RESOURCE,
	  0,
	  D3D11_RESOURCE_MISC_TEXTURECUBE,
	  DirectX::DDS_LOADER_DEFAULT,
	  &resource,
	  &texture
	) != S_OK)
	throw std::runtime_error("Could not load texture " + pyr::widestring2string(path));

  return Cubemap(resource, texture);
}

void Texture::releaseRawTexture()
{
  DXRelease(m_resource);
  DXRelease(m_texture);
  m_width = m_height = 0;
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
