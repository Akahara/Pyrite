#pragma once

#include "display/ConstantBuffer.h"

namespace pyr
{
	using CameraBuffer = pyr::ConstantBuffer < InlineStruct(mat4 mvp; alignas(16) vec3 pos) > ;
	using InverseCameraBuffer = pyr::ConstantBuffer < InlineStruct(mat4 inverseViewProj;  mat4 inverseProj; alignas(16) mat4 Proj) > ;
	using ActorBuffer = ConstantBuffer < InlineStruct(mat4 modelMatrix) >;
}