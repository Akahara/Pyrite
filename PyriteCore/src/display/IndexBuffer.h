#pragma once
#include <cstdint>
#include <vector>
#include <span>

struct ID3D11Buffer;

namespace pyr
{
    
class IndexBuffer
{

public:
	using size_type = uint32_t;

private:

	size_t m_indiceCount;

	ID3D11Buffer* m_ibo = nullptr;

public:

	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// -- Basic operations

	[[nodiscard]] size_t					getIndicesCount() const noexcept;
	void bind() const;

	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// -- Constructors and other stuff
	///
	IndexBuffer() = default;
	explicit IndexBuffer(std::span<const size_type> indices);
	explicit IndexBuffer(const std::vector<size_type>& indices) : IndexBuffer(std::span{ indices }){}

	void swap(IndexBuffer& other) noexcept;
	IndexBuffer(const IndexBuffer&) = delete;
	IndexBuffer& operator=(const IndexBuffer&) = delete;

	IndexBuffer(IndexBuffer&& other) noexcept;
	IndexBuffer& operator=(IndexBuffer&& other) noexcept;
	~IndexBuffer();
    
};
}
