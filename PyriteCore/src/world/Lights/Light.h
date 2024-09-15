#pragma once

#include "utils/Math.h"
#include "utils/Debug.h"

#include <iostream>

using namespace DirectX;


static inline PYR_DEFINELOG(LogLights, VERBOSE);

namespace pyr {

/// =============================================================================================================================================================///

enum LightTypeID : uint32_t
{
	Directional = 1,
	Point = 2,
	Spotlight = 3,
};

struct hlsl_GenericLight
{

	vec4	direction; // For directional lights and spotlight
	vec4	range;
	vec4	position;
	vec4	ambiant;
	vec4	diffuse;

	float specularFactor;
	float fallOff;
	float strength;
	float isOn;

	LightTypeID type;
	float padding[3];
};

/// =============================================================================================================================================================///

struct BaseLight
{
	bool isOn = false;
	vec4 ambiant;
	vec4 diffuse;
};

template<class L> requires std::derived_from<L, BaseLight>
hlsl_GenericLight convertLightTo_HLSL(const L& light);


/// =============================================================================================================================================================///

struct DirectionalLight : public BaseLight {

	vec4 direction = { 0.f,0.f,-1.f,0.f };
	float strength = 1.0F;
};

template<>
hlsl_GenericLight convertLightTo_HLSL<DirectionalLight>(const DirectionalLight& light)
{
	return hlsl_GenericLight{
		.direction = light.direction,
		.range = {},
		.position = {},
		.ambiant = light.ambiant,
		.diffuse = light.diffuse,
		.specularFactor = {},
		.fallOff = {},
		.strength = light.strength,
		.isOn = static_cast<float>(light.isOn),
		.type = LightTypeID::Directional
	};
}

/// =============================================================================================================================================================///


struct PointLight : public BaseLight {

	int distance = 7; // lookup table
	vec4 range = computeRangeFromDistance(7);
	vec4 position;
	float specularFactor = 1.0;

	PointLight() = default;
	PointLight(unsigned int d, vec4 pos, vec4 Ka, vec4 Kd, float f, bool on)
		: BaseLight{ .isOn = on, .ambiant = Ka, .diffuse = Kd},
		distance(d), range(computeRangeFromDistance(distance)), position(pos), 
		specularFactor(f)
	{}

	vec4 computeRangeFromDistance(unsigned int distance)
	{
		static vec4 lookup[] =
		{
			// dist  const  linear   quad
			{7		,1.0f	,0.7f	,1.8f},
			{13		,1.0f	,0.35f	,0.44f},
			{20		,1.0f	,0.22f	,0.20f},
			{32		,1.0f	,0.14f	,0.07f},
			{50		,1.0f	,0.09f	,0.032f},
			{65		,1.0f	,0.07f	,0.017f},
			{100	,1.0f	,0.045f	,0.0075f},
			{160	,1.0f	,0.027f	,0.0028f},
			{200	,1.0f	,0.022f	,0.0019f},
			{325	,1.0f	,0.014f	,0.0007f},
			{600	,1.0f	,0.007f	,0.0002f},
			{3250	,1.0f	,0.0014f,0.000007f}
		};

		return lookup[std::clamp(distance, 0u, 11u)];

	}

};

template<>
hlsl_GenericLight convertLightTo_HLSL<PointLight>(const PointLight& light)
{
	return hlsl_GenericLight{
		.direction = {},
		.range = light.range,
		.position = light.position,
		.ambiant = light.ambiant,
		.diffuse = light.diffuse,
		.specularFactor = light.specularFactor,
		.fallOff = {},
		.strength = {},
		.isOn = static_cast<float>(light.isOn),
		.type = LightTypeID::Point
	};
}

/// =============================================================================================================================================================///

struct SpotLight : public  BaseLight {

	vec4 direction = { 0.f,0.f,-1.f,0.f };
	vec4 position;

	float outsideAngle = 1.6f;
	float insideAngle = 0.5f;
	float strength = 1.0f;
	float specularFactor = 1.0f;
};

template<>
hlsl_GenericLight convertLightTo_HLSL<SpotLight>(const SpotLight& light)
{
	return hlsl_GenericLight{
		.direction = light.direction,
		.range = {light.insideAngle,light.insideAngle,light.insideAngle,light.insideAngle},
		.position = light.position,
		.ambiant = light.ambiant,
		.diffuse = light.diffuse,
		.specularFactor = light.specularFactor,
		.fallOff = light.outsideAngle,
		.strength = light.strength,
		.isOn = static_cast<float>(light.isOn),
		.type = LightTypeID::Spotlight
	};
}

/// =============================================================================================================================================================///

///  Instanciate this into your scene, should have one in the editor part

/// =============================================================================================================================================================///

struct LightsCollections {

	std::vector<SpotLight> Spots;
	std::vector<PointLight> Points;
	std::vector<DirectionalLight> Directionals;

	template<size_t N = 16>
	std::array<hlsl_GenericLight, N> ConvertCollectionToHLSL()
	{
		std::array<hlsl_GenericLight, N> res;

		auto it = res.begin();
		for (const SpotLight& spot : Spots)
			*(it++) = convertLightTo_HLSL<SpotLight>(spot);
		for (const PointLight& point : Points)
			*(it++) = convertLightTo_HLSL<PointLight>(point);
		for (const DirectionalLight& dir : Directionals)
			*(it++) = convertLightTo_HLSL<DirectionalLight>(dir);

		return res;
	}

	void RemoveLight(BaseLight* light)
	{
		{
			const auto [first,last] = std::ranges::remove_if(Spots, [light](SpotLight& l) { return &l == light; });
			Spots.erase(first, last);
		}
		{
			const auto [first, last] = std::ranges::remove_if(Points, [light](PointLight& l) { return &l == light; });
			Points.erase(first, last);
		}
		{
			const auto [first, last] = std::ranges::remove_if(Directionals, [light](DirectionalLight& l) { return &l == light; });
			Directionals.erase(first, last);
		}
	}

};



}