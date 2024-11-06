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

Texture TextureManager::loadTexture(const std::wstring &path, bool bGenerateMips /* = true */)
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
		  bGenerateMips  ? &Engine::d3dcontext() : nullptr,
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
		  bGenerateMips ? &Engine::d3dcontext() : nullptr,
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

void Cubemap::releaseRawCubemap()
{
	DXRelease(m_resource);
	DXRelease(m_texture);
}


TextureArray::TextureArray(size_t width, size_t height, size_t count /*= 8*/, bool bIsDepthOnly /* = false */, bool bIsCubeArray /*= false*/)
	: m_width(width)
	, m_height(height)
	, m_arrayCount(bIsCubeArray ? count : count * 6)
	, m_isCubeArray(bIsCubeArray)
{
	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = m_arrayCount;
	texDesc.Format = bIsDepthOnly ? DXGI_FORMAT_R32_FLOAT : DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	if (bIsCubeArray) texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	// Create the texture array resource
	ID3D11Texture2D* texture;
	HRESULT hr = pyr::Engine::d3ddevice().CreateTexture2D(&texDesc, nullptr, &texture);
	m_resource = texture;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = texDesc.Format;

	if (bIsCubeArray)
	{
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
		srvDesc.TextureCubeArray.MostDetailedMip = 0;
		srvDesc.TextureCubeArray.MipLevels = texDesc.MipLevels;
		srvDesc.TextureCubeArray.First2DArrayFace = 0;
		srvDesc.TextureCubeArray.NumCubes = m_arrayCount / 6;
	}
	else
	{
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.MipLevels = texDesc.MipLevels;
		srvDesc.Texture2DArray.FirstArraySlice = 0;
		srvDesc.Texture2DArray.ArraySize = texDesc.ArraySize;
	}

	hr = pyr::Engine::d3ddevice().CreateShaderResourceView(texture, &srvDesc, &m_textureArray);
}

TextureArray::~TextureArray()
{
	DXRelease(m_textureArray);
	DXRelease(m_resource);
}

void TextureArray::CopyToTextureArray(const std::vector<Texture>& textures, TextureArray& outArray)
{
	if (textures.empty()) return;
	//if (!PYR_ENSURE(textures.size() == outArray.getArrayCount())) return;
	if (!PYR_ENSURE(textures[0].getWidth() == outArray.getWidth())) return;
	if (!PYR_ENSURE(textures[0].getHeight() == outArray.getHeight())) return;

	ID3D11Texture2D* resource = static_cast<ID3D11Texture2D*>(outArray.getRawResource());

	for (UINT i = 0; i < outArray.getArrayCount() && i < textures.size(); ++i)
	{
		// Copy the texture to the corresponding slice in the array
		pyr::Engine::d3dcontext().CopySubresourceRegion(
			resource,
			D3D11CalcSubresource(0, i, 1),		// MipSlice = 0, ArraySlice = i
			0, 0, 0,                            // Destination X, Y, Z
			textures[i].getRawResource(), 
			0,                                  // Source subresource index
			nullptr);                           // No source box, copy entire resource
	}
}

void TextureArray::CopyToTextureArray(const std::vector<Cubemap>& cubemaps, TextureArray& outArray)
{
	if (cubemaps.empty()) return;
	// Ensure cubemaps and outArray dimensions match
	//if (!PYR_ENSURE(cubemaps[0].getWidth() == outArray.getWidth())) return;
	//if (!PYR_ENSURE(cubemaps[0].getHeight() == outArray.getHeight())) return;

	ID3D11Texture2D* resource = static_cast<ID3D11Texture2D*>(outArray.getRawResource());

	// Iterate through each cubemap
	for (UINT i = 0; i < outArray.getArrayCount() && i < cubemaps.size(); ++i)
	{
		// We are copying cubemap faces to the corresponding slice in the array
		for (UINT face = 0; face < 6; ++face)
		{
			// Calculate the slice index for this cubemap face
			UINT sliceIndex = i * 6 + face;

			// Copy the individual face to the corresponding slice in the cubemap array
			pyr::Engine::d3dcontext().CopySubresourceRegion(
				resource,
				D3D11CalcSubresource(0, sliceIndex, 1), // MipSlice = 0, ArraySlice = sliceIndex
				0, 0, 0,                                // Destination X, Y, Z
				cubemaps[i].getRawResource(),           // Source cubemap resource
				face,                                   // Copy the specific face (0-5 for cubemap faces)
				nullptr);                               // No source box, copy the entire resource
		}
	}
}


} // end namespace pyr
