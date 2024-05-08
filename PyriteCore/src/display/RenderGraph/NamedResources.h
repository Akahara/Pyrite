#pragma once
#include <string>

namespace pyr
{
    struct NamedResource
    {
        using label_t = std::string;
        std::string label;
    };

    struct NamedOutput : NamedResource {};
    struct NamedInput : NamedResource {};

}
