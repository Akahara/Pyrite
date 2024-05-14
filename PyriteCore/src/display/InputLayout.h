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
        template<class T, class I=EmptyVertex> requires std::derived_from<T, BaseVertex> and std::derived_from<I, BaseVertex>
        static InputLayout MakeLayoutFromVertex()
        {
            std::vector<D3D11_INPUT_ELEMENT_DESC> desc;

            auto appendLayoutItem = [&]<class It>(It, bool bInstanced) constexpr
            {
                PYR_ASSERT(It::bIsInstanced == bInstanced, "Either you put vertex data in the instance buffer or instance data in the vertex buffer"); // cannot be made a static assertion because of the compiler
                UINT rowCount = sizeof(It) == sizeof(mat4) ? 4 : 1;
                DXGI_FORMAT format = sizeof(It) == sizeof(mat4) ? DXGI_FORMAT_R32G32B32A32_FLOAT : formats[sizeof(It) / sizeof(float) - 1];
                for (UINT j = 0; j < rowCount; j++) {
                    desc.push_back(D3D11_INPUT_ELEMENT_DESC{
                        .SemanticName = It::semanticName,
                        .SemanticIndex = j,
                        .Format = format,
                        .InputSlot = bInstanced ? 1u : 0u,
                        .AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
                        .InputSlotClass = bInstanced ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA,
                        .InstanceDataStepRate = bInstanced ? 1u : 0u,
                    });
                }
            };

            auto appendLayout = [&]<class ...Ts>(std::tuple<Ts...>, bool bInstanced) constexpr { (appendLayoutItem(Ts{}, bInstanced), ...); };
            appendLayout(typename T::type_t{}, false);
            appendLayout(typename I::type_t{}, true);

            return InputLayout{ std::move(desc) };
        }
    };
}
