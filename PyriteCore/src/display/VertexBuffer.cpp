#include "VertexBuffer.h"

#include <d3d11.h>
#include <vector>
#include <algorithm>

namespace pyr
{
    
void VertexBuffer::bind() const noexcept
{
	constexpr UINT offset = 0;
	pyr::Engine::d3dcontext().IASetVertexBuffers(0, 1, &m_vbo, &m_stride, &offset);
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