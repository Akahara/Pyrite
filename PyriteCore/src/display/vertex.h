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
		INSTANCE_COLOR,
		INSTANCE_TRANSFORM,
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////
	template <VertexParameterType T>
	struct VertexParameter {};

	template <> struct VertexParameter<POSITION> {
	    static constexpr const char* semanticName = "POSITION";
		static constexpr bool bIsInstanced = false;
		alignas(sizeof(vec4)) vec4 position;
	};

	template <> struct VertexParameter<NORMAL> {
	    static constexpr const char* semanticName = "NORMAL";
		static constexpr bool bIsInstanced = false;
		alignas(sizeof(vec4)) vec3 normal;
	};

	template <> struct VertexParameter<TANGENT> {
	    static constexpr const char* semanticName = "TANGENT";
		static constexpr bool bIsInstanced = false;
		alignas(sizeof(vec4)) vec3 tangent;
	};

	template <> struct VertexParameter<UV> {
	    static constexpr const char* semanticName = "UV";
		static constexpr bool bIsInstanced = false;
		alignas(sizeof(vec2)) vec2 texCoords;
	};

	template <> struct VertexParameter<COLOR> {
	    static constexpr const char* semanticName = "COLOR";
		static constexpr bool bIsInstanced = false;
	    alignas(sizeof(vec4)) vec4 color;
	};

	template <> struct VertexParameter<INSTANCE_COLOR> {
	    static constexpr const char* semanticName = "INSTANCE_COLOR";
	    static constexpr bool bIsInstanced = true;
	    alignas(sizeof(vec4)) vec4 instanceColor;
	};

	template <> struct VertexParameter<INSTANCE_TRANSFORM> {
	    static constexpr const char* semanticName = "INSTANCE_TRANSFORM";
	    static constexpr bool bIsInstanced = true;
	    alignas(sizeof(vec4)) mat4 instanceTransform;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////

	struct BaseVertex
	{
	    // Adapt a 3-component vector to the standard 4-component vector that shaders use
	    static vec4 toPosition(const vec3& p) { return { p.x, p.y, p.z, 1.f }; }
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
