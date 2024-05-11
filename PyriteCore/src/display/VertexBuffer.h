#pragma once

#include <d3d11.h>
#include <vector>

#include "Vertex.h"
#include "engine/Engine.h"

namespace pyr
{

    class VertexBuffer
    {
        size_t          m_vertexCount;
        UINT            m_stride;
        ID3D11Buffer*   m_vbo = nullptr;

    public:

        template<class V> requires std::derived_from<V, BaseVertex>
        explicit VertexBuffer(const std::vector<V>& vertices, bool bMutable=false) : m_stride(sizeof(V))
        {
            m_vertexCount = vertices.size();

            D3D11_BUFFER_DESC descriptor{};
            D3D11_SUBRESOURCE_DATA initData{};

            // -- Vertex buffer
            ZeroMemory(&descriptor, sizeof(descriptor));

            descriptor.Usage = bMutable ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DYNAMIC;
            descriptor.ByteWidth = static_cast<UINT>(vertices.size()) * sizeof(V);
            descriptor.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            descriptor.CPUAccessFlags = 0;

            ZeroMemory(&initData, sizeof(initData));
            initData.pSysMem = &vertices[0];

            Engine::d3ddevice().CreateBuffer(&descriptor, &initData, &m_vbo);
        }


        /////////////////////////////////////////////////////////////////////////////////////////////////////////

        [[nodiscard]] size_t getVerticesCount()			const noexcept { return m_vertexCount; }
        [[nodiscard]] UINT   getStride()			    const noexcept { return m_stride; }
        void setData(const void* data, size_t size, size_t offset);
        void bind() const noexcept;

        /////////////////////////////////////////////////////////////////////////////////////////////////////////

        VertexBuffer() = default;
        void swap(VertexBuffer& other) noexcept;
        VertexBuffer(const VertexBuffer&) = delete;
        VertexBuffer& operator=(const VertexBuffer&) = delete;
        VertexBuffer(VertexBuffer&& other) noexcept;
        VertexBuffer& operator=(VertexBuffer&& other) noexcept;
        ~VertexBuffer();
    };
}
