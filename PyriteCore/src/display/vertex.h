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

	template <typename T>
	struct vpt_traits;

	/////////////////////////////////////////////////////////////////////////////////////////////////
	template <VertexParameterType T>
	struct VertexParameter
	{
	};

	template<VertexParameterType T>
	struct vpt_traits<VertexParameter<T>> {
	  static constexpr VertexParameterType type = T;
	};

	template <>
	struct VertexParameter<POSITION>
	{
		alignas(sizeof(vec4)) vec4 position;
	};

	template <>
	struct VertexParameter<NORMAL>
	{
		alignas(sizeof(vec4)) vec3 normal;
	};

	template <>
	struct VertexParameter<TANGENT>
	{
		alignas(sizeof(vec4)) vec3 tangent;
	};

	template <>
	struct VertexParameter<UV>
	{
		alignas(sizeof(vec2)) vec2 texCoords;
	};

	template <>
	struct VertexParameter<COLOR>
    {
	  alignas(sizeof(vec4)) vec4 color;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////

	struct BaseVertex
	{
	    // Adapt a 3-component vector to the standard 4-component vector that shaders use
	    static vec4 toPosition(const vec3& p) { return { p.x, p.y, p.z, 1.f };}
	};


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
