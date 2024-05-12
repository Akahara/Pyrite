#pragma once

#include <array>
#include <span>
#include <unordered_map>

#include "Vertex.h"
#include "engine/Directxlib.h"

namespace pyr
{
    // This class produces an object that describes the layout for a vertex in the shader pipeline.
    // This is meant to be used as a replacement for :
    // 
    //	D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
    //	{
    //		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    //		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    //	};
    // 

 
    inline std::unordered_map<VertexParameterType, LPCSTR> labelMap = {
    { NORMAL, "NORMAL"},
    { POSITION, "POSITION"},
    { TANGENT, "TANGENT"},
    { UV, "TEXCOORD"},
    { COLOR, "COLOR" },
    //"BLENDINDICES",
    //"BLENDWEIGHT",
    //"BINORMAL",
    //"POSITION",
    //"POSITIONT",
    //"PSIZE",
    //"TANGENT",
    //"TEXCOORD",
    //"COLOR"
    };

    inline const std::array<DXGI_FORMAT, 4> formats {
        DXGI_FORMAT_R32_FLOAT,
        DXGI_FORMAT_R32G32_FLOAT,
        DXGI_FORMAT_R32G32B32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT
    };

    class InputLayout
    {
    private:
        std::vector<D3D11_INPUT_ELEMENT_DESC> elements;

    public:
        InputLayout() = default;
        InputLayout(const std::vector<D3D11_INPUT_ELEMENT_DESC>& elements) : elements{ elements }
        {}

        operator        D3D11_INPUT_ELEMENT_DESC* ()                    { return elements.data();     }
        operator const	D3D11_INPUT_ELEMENT_DESC* () const              { return elements.data();     }
        operator        std::vector<D3D11_INPUT_ELEMENT_DESC>& ()       { return elements;            }
        operator const	std::vector<D3D11_INPUT_ELEMENT_DESC>& () const { return elements;            }
        operator        std::span<D3D11_INPUT_ELEMENT_DESC>()           { return std::span(elements); }

        bool empty() const  { return elements.empty(); }
        size_t count() const { return elements.size(); }
        const D3D11_INPUT_ELEMENT_DESC* data() const { return elements.data(); }
        D3D11_INPUT_ELEMENT_DESC* data() { return elements.data(); }

    public:
        // Compile time input layout
        template<class T> requires std::derived_from<T, BaseVertex>
        static InputLayout MakeLayoutFromVertex(bool bIsInstanced = false)
        {
            int i = 0;
            std::array<D3D11_INPUT_ELEMENT_DESC, std::tuple_size_v<typename T::type_t>> desc{};
            std::apply([&]<class ...Ts>(Ts&&...) constexpr {
                ((desc[i++] = D3D11_INPUT_ELEMENT_DESC{
                    labelMap[vpt_traits<Ts>::type],
                    0,
                    formats[sizeof(Ts) / sizeof(float) - 1],
                    0,
                    D3D11_APPEND_ALIGNED_ELEMENT,
                    (bIsInstanced)
                        ? D3D11_INPUT_PER_INSTANCE_DATA
                        : D3D11_INPUT_PER_VERTEX_DATA,
                    0
                    }), ...);
            }, typename T::type_t{});

            return InputLayout{ {desc.begin(), desc.end()} };
        }
    };
}
