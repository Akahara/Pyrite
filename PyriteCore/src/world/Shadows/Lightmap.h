#pragma once

#include "display/texture.h"

namespace pyr
{

class Lightmap
{

private:

	Texture m_texture;

public:

	Texture GetTexture() const { return m_texture; }

};

}
