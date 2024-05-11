#pragma once

#include "utils/math.h"

namespace pyr
{
	/////////////////////////////////////////////////////////////////////////////////////////////////

	enum VertexParameterType : uint8_t
	{
		POSITION,
		NORMAL,
		TANGENT,
		UV,
		COLOR,
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
		vec3 position;
	};

	template <>
	struct VertexParameter<NORMAL>
	{
		static constexpr VertexParameterType type = NORMAL;
		vec3 normal;
	};

	template <>
	struct VertexParameter<TANGENT>
	{
		static constexpr VertexParameterType type = TANGENT;
		vec3 tangent;
	};

	template <>
	struct VertexParameter<UV>
	{
		static constexpr VertexParameterType type = UV;
		vec2 texCoords;
	};

	template <>
	struct VertexParameter<COLOR> {
	  static constexpr VertexParameterType type = UV;
	  vec4 color;
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
