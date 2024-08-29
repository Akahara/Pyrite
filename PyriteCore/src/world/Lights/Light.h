#pragma once

#include "utils/Math.h"
#include "utils/Debug.h"

#include <iostream>

using namespace DirectX;


static inline PYR_DEFINELOG(LogLights, VERBOSE);

namespace pyr {

/// =============================================================================================================================================================///


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

	uint32_t type; // 1 : dir, 2 : point , 3 : dir
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
		light.direction,
		{},
		{},
		light.ambiant,
		light.diffuse,
		{},
		{},
		light.strength,
		static_cast<float>(light.isOn),
		1
	};
}

/// =============================================================================================================================================================///


struct PointLight : public BaseLight {

	int distance = 7;
	vec4 range = computeRangeFromDistance(7);
	vec4 position;
	float specularFactor = 1.0;

	PointLight() = default;
	PointLight(unsigned int d, vec4 pos, vec4 Ka, vec4 Kd, float f, bool on)
	{
		distance = d;
		range = computeRangeFromDistance(distance);
		position = pos;
		ambiant = Ka;
		diffuse = Kd;
		specularFactor = f;
		isOn = on;
	}

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
		{},
		light.range,
		light.position,
		light.ambiant,
		light.diffuse,
		light.specularFactor,
		{},
		{},
		static_cast<float>(light.isOn),
		2
	};
}

/// =============================================================================================================================================================///

struct SpotLight : public  BaseLight {

	vec4 direction = { 0.f,0.f,-1.f,0.f };
	vec4 position;

	float outsideAngle = 1.6;
	float insideAngle = 0.5;
	float strength = 1.0;
	float specularFactor = 1.0;
};

template<>
hlsl_GenericLight convertLightTo_HLSL<SpotLight>(const SpotLight& light)
{
	return hlsl_GenericLight{
		light.direction,
		{light.insideAngle,light.insideAngle,light.insideAngle,light.insideAngle},
		light.position,
		light.ambiant, light.diffuse,
		light.specularFactor,
		light.outsideAngle,
		light.strength,
		static_cast<float>(light.isOn),
		3
	};
}

/// =============================================================================================================================================================///

///  Instanciate this into your scene, should have one in the editor part

/// =============================================================================================================================================================///

struct LightsCollections {

	std::vector<SpotLight> Spots;
	std::vector<PointLight> Points;
	std::vector<DirectionalLight> Directionals;

	std::vector<hlsl_GenericLight> ConvertCollectionToHLSL(size_t desiredMinCapacity = 16)
	{
		std::vector<hlsl_GenericLight> res;
		res.resize(desiredMinCapacity);

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