#include "IndexBuffer.h"


#include <d3d11.h>
#include <vector>
#include <algorithm>

#include "engine/Directxlib.h"
#include "engine/Engine.h"

namespace pyr
{
IndexBuffer::IndexBuffer(const std::vector<size_type>& indices)
{

	m_indiceCount = indices.size();

	D3D11_BUFFER_DESC m_descriptor{};
	D3D11_SUBRESOURCE_DATA m_initData{};

	// -- Index buffer

	ZeroMemory(&m_descriptor, sizeof(m_descriptor));

	m_descriptor.Usage = D3D11_USAGE_IMMUTABLE;
	m_descriptor.ByteWidth = static_cast<UINT>(indices.size()) * sizeof(size_type);
	m_descriptor.BindFlags = D3D11_BIND_INDEX_BUFFER;
	m_descriptor.CPUAccessFlags = 0;

	ZeroMemory(&m_initData, sizeof(m_initData));
	m_initData.pSysMem = &indices[0];

	Engine::d3ddevice().CreateBuffer(&m_descriptor, &m_initData, &m_ibo);
}

void IndexBuffer::swap(IndexBuffer& other) noexcept {
	std::swap(other.m_ibo, m_ibo);
	std::swap(other.m_indiceCount, m_indiceCount);
}


size_t IndexBuffer::getIndicesCount() const noexcept { return m_indiceCount; }

void IndexBuffer::bind() const
{
	Engine::d3dcontext().IASetIndexBuffer(m_ibo, DXGI_FORMAT_R32_UINT, 0);
}

IndexBuffer::IndexBuffer(IndexBuffer&& other) noexcept
	: m_ibo(std::exchange(other.m_ibo, nullptr))
    , m_indiceCount(std::exchange(other.m_indiceCount, {})) 
{	}

IndexBuffer& IndexBuffer::operator=(IndexBuffer&& other) noexcept
{
	IndexBuffer{ std::move(other) }.swap(*this);
	return *this;
}

IndexBuffer::~IndexBuffer()
{
	DXRelease(m_ibo);
}
}