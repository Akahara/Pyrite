#pragma once

#include "display/texture.h"
#include "world/camera.h"
#include "display/FrameBuffer.h"

#include <filesystem>
#include <vector>
#include <array>

class Scene;

namespace pyr
{
	class CubemapBuilder
	{

	public:

		template<size_t mipCount>
		static Cubemap MakeCubemapFromTexturesLOD(const std::span<pyr::Texture>& textures);

		static Cubemap MakeCubemapFromTextures(std::array<pyr::Texture, 6> textures)
		{
			return CubemapBuilder::MakeCubemapFromTexturesLOD<1>(textures);
		}

	};

	template<size_t M>
	inline Cubemap CubemapBuilder::MakeCubemapFromTexturesLOD(const std::span<pyr::Texture>& textures)
	{
		// -- Ensure texture count 
		UINT mipCount = M;
		assert(textures.size() == mipCount * 6); 
		
		// -- Ensure main texture is squared
		assert(textures[0].getWidth() == textures[0].getHeight());
		auto width = static_cast<UINT>(textures[0].getWidth());

		// -- Describe the texture cubemap
		D3D11_TEXTURE2D_DESC textureDesc = {};
		textureDesc.Width = width;
		textureDesc.Height = width;
		textureDesc.MipLevels = static_cast<UINT>(mipCount);
		textureDesc.ArraySize = 6;
		textureDesc.Format = DXGI_FORMAT_R11G11B10_FLOAT;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		textureDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
		textureDesc.CPUAccessFlags = 0;
		
		// -- Create the result texture
		ID3D11Texture2D* cubeTexture = nullptr;
		DXTry(Engine::d3ddevice().CreateTexture2D(&textureDesc, nullptr, &cubeTexture), "Failed to create a texture 2D");
		
		// -- For each face
		for (UINT faceID = 0; faceID  < 6; ++faceID) {
			
			// -- For each mip level
			for (UINT mipLevel = 0; mipLevel < mipCount; mipLevel++)
			{
				UINT subresource = D3D11CalcSubresource(mipLevel, faceID, mipCount);
				size_t texID = mipLevel * 6 + faceID;
				ID3D11Texture2D* faceTexture = static_cast<ID3D11Texture2D*>(textures[texID].getRawResource());
				Engine::d3dcontext().CopySubresourceRegion(cubeTexture, subresource, 0, 0, 0, faceTexture, 0, nullptr);
			}
		}

		// -- Describe the SRV
		D3D11_SHADER_RESOURCE_VIEW_DESC SMViewDesc = {};
		SMViewDesc.Format = textureDesc.Format;
		SMViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		SMViewDesc.TextureCube.MipLevels = static_cast<UINT>(mipCount);
		SMViewDesc.TextureCube.MostDetailedMip = 0;

		ID3D11ShaderResourceView* shaderResourceView = nullptr;
		DXTry(Engine::d3ddevice().CreateShaderResourceView(cubeTexture, &SMViewDesc, &shaderResourceView), "Could not create a srv");

		return Cubemap(cubeTexture, shaderResourceView);
	}

}