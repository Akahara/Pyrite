#include "VertexBuffer.h"

#include <d3d11.h>
#include <vector>
#include <algorithm>

namespace pyr
{
void VertexBuffer::setData(const void* data, size_t size, size_t offset)
{
  D3D11_MAPPED_SUBRESOURCE mappedResource;
  ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));

  DXTry(Engine::d3dcontext().Map(m_vbo, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource), "Could not map a vertex buffer");
  memcpy(static_cast<char*>(mappedResource.pData) + offset, data, size);
  Engine::d3dcontext().Unmap(m_vbo, 0);
}

void VertexBuffer::bind() const noexcept
{
	constexpr UINT offset = 0;
    Engine::d3dcontext().IASetVertexBuffers(0, 1, &m_vbo, &m_stride, &offset);
}

void VertexBuffer::swap(VertexBuffer& other) noexcept {
	std::swap(other.m_vbo, m_vbo);
	std::swap(other.m_stride, m_stride);
	std::swap(other.m_vertexCount, m_vertexCount);
}

VertexBuffer::VertexBuffer(VertexBuffer&& other) noexcept
	: m_vbo(std::exchange(other.m_vbo, nullptr))
	, m_stride(std::exchange(other.m_stride, {}))
	, m_vertexCount(std::exchange(other.m_vertexCount, {}))
{}

VertexBuffer& VertexBuffer::operator=(VertexBuffer&& other) noexcept
{
	VertexBuffer{ std::move(other) }.swap(*this);
	return *this;
}

VertexBuffer::~VertexBuffer()
{
	DXRelease(m_vbo);
}

}