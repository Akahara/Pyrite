#pragma once
#include <string>

#include "display/texture.h"

namespace pyr
{
    class RenderPass;
   
    struct NamedResource
    {
        using label_t = std::string;
        std::string label;

        Texture res; // fuck this shit man
        RenderPass* origin;
    };

    using NamedOutput = NamedResource;
    using NamedInput = NamedResource;
}
