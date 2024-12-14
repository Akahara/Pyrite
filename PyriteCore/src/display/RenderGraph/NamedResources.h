#pragma once

#include <functional>
#include <string>
#include <variant>

#include "display/texture.h"

namespace pyr
{
    class RenderPass;
   
    struct NamedResource
    {
        using resource_t = std::variant<Texture, Cubemap, TextureArray>;
        std::string label;

        resource_t res; // fuck this shit man
        RenderPass* origin;
    };

    using NamedOutput = NamedResource;
    using NamedInput = NamedResource;
    using ResourceGetter = std::function<NamedResource::resource_t()>;

}
