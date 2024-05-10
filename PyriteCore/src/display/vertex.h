#pragma once

#include "directtk/SimpleMath.h"

using namespace DirectX::SimpleMath;

namespace pyr
{
	/////////////////////////////////////////////////////////////////////////////////////////////////

	enum VertexParameterType : uint8_t
	{
		POSITION,
		NORMAL,
		TANGENT,
		UV,
	};

	template <VertexParameterType T>
	struct vpt_traits
	{
		static constexpr VertexParameterType type = T;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////
	template <VertexParameterType T>
	struct VertexParameter
	{
		static constexpr VertexParameterType type = T;
	};

	template <>
	struct VertexParameter<POSITION>
	{
		static constexpr VertexParameterType type = POSITION;
		Vector3 position;
	};

	template <>
	struct VertexParameter<NORMAL>
	{
		static constexpr VertexParameterType type = NORMAL;
		Vector3 normal;
	};

	template <>
	struct VertexParameter<TANGENT>
	{
		static constexpr VertexParameterType type = TANGENT;
		Vector3 tangent;
	};

	template <>
	struct VertexParameter<UV>
	{
		static constexpr VertexParameterType type = UV;
		Vector2 texCoords;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////

	struct BaseVertex
	{};


	// Feed multiple vertex parameter type :
	// GenericVertex<POSITION, COLOR...>;
	template <VertexParameterType ... M>
	struct GenericVertex : VertexParameter<M>..., BaseVertex
	{
		using type_t = std::tuple<VertexParameter<M>...>;
	};

	using EmptyVertex = GenericVertex<>;

	/////////////////////////////////////////////////////////////////////////////////////////////////
}
