#pragma once

#include <vector>
#include <span>

#include "Vertex.h"
#include "engine/Engine.h"
#include "utils/debug.h"

namespace pyr
{

    class VertexBuffer
    {
        size_t          m_vertexCount;
        UINT            m_stride;
        ID3D11Buffer*   m_vbo = nullptr;

    public:

        template<class V> requires std::derived_from<V, BaseVertex>
        explicit VertexBuffer(std::span<V> vertices, bool bMutable=false) : m_stride(sizeof(V))
        {
            PYR_ASSERT(vertices.size() > 0);

            m_vertexCount = vertices.size();

            D3D11_BUFFER_DESC descriptor{};
            D3D11_SUBRESOURCE_DATA initData{};

            // -- Vertex buffer
            ZeroMemory(&descriptor, sizeof(descriptor));

            descriptor.Usage = bMutable ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_IMMUTABLE;
            descriptor.ByteWidth = static_cast<UINT>(vertices.size()) * sizeof(V);
            descriptor.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            descriptor.CPUAccessFlags = bMutable ? D3D11_CPU_ACCESS_WRITE : 0;

            ZeroMemory(&initData, sizeof(initData));
            initData.pSysMem = &vertices[0];

            Engine::d3ddevice().CreateBuffer(&descriptor, &initData, &m_vbo);
        }

        template<class V> requires std::derived_from<V, BaseVertex>
        explicit VertexBuffer(const std::vector<V>& vertices, bool bMutable = false) : VertexBuffer(std::span{ vertices }, bMutable)
        {}


        /////////////////////////////////////////////////////////////////////////////////////////////////////////

        [[nodiscard]] size_t getVerticesCount()			const noexcept { return m_vertexCount; }
        [[nodiscard]] UINT   getStride()			    const noexcept { return m_stride; }
        void setData(const void* data, size_t size, size_t offset);
        void bind(bool bAsInstanceBuffer=false) const noexcept;

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
