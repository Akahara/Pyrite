#pragma once

#include "display/texture.h"
#include <variant>

template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

namespace pyr
{

class Lightmap
{
public:
	using lightmap_t = std::variant<Texture, Cubemap>;
private:

	lightmap_t m_resource;

public:

	ID3D11ShaderResourceView* GetTexture() const { return std::visit<ID3D11ShaderResourceView*>(overloaded{
		[](const Texture& asTexture) { return asTexture.getRawTexture(); },
		[](const Cubemap& asCubemap) { return asCubemap.getRawCubemap(); },
		},
		m_resource); }



};

}
