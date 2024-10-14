#pragma once

#include "utils/Math.h"
#include "utils/Debug.h"
#include "world/Actor.h"

#include <iostream>

using namespace DirectX;


static inline PYR_DEFINELOG(LogLights, VERBOSE);

// TODO : Make light a variant

namespace pyr {

/// =============================================================================================================================================================///

enum LightTypeID : uint32_t
{
	Undefined = 0,
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

struct BaseLight : public Actor
{
	bool isOn = true;
	vec4 ambiant;
	vec4 diffuse = {1,1,1,1};
	BaseLight()
	{
		GetTransform().rotation = vec4{ 0,-1,0,0 }; // todo make this a directional arrow widget someday ? and make this cleaner
	}
	BaseLight(bool isOn, vec4 ambiant, vec4 diffuse) : isOn(isOn), ambiant(ambiant), diffuse(diffuse)
	{
		GetTransform().rotation = vec4{ 0,-1,0,0 };
	}

	virtual LightTypeID getType() const { return Undefined; }
};

template<class L> requires std::derived_from<L, BaseLight>
inline hlsl_GenericLight convertLightTo_HLSL(const L& light);


/// =============================================================================================================================================================///

struct DirectionalLight : public BaseLight {

	float strength = 1.0F;
	virtual LightTypeID getType() const override final { return Directional; }
};

template<>
inline hlsl_GenericLight convertLightTo_HLSL<DirectionalLight>(const DirectionalLight& light)
{
	return hlsl_GenericLight{
		.direction = light.GetTransform().rotation,
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
	float specularFactor = 1.0;

	PointLight() = default;
	PointLight(unsigned int d, vec3 pos, vec4 Ka, vec4 Kd, float f, bool on)
		: BaseLight{ on, Ka, Kd},
		distance(d), range(computeRangeFromDistance(distance)), 
		specularFactor(f)
	{
		GetTransform().position = pos;
	}

	virtual LightTypeID getType() const override final { return Point; }

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
inline hlsl_GenericLight convertLightTo_HLSL<PointLight>(const PointLight& light)
{
	return hlsl_GenericLight{
		.direction = {},
		.range = light.range,
		.position = vec4{light.GetTransform().position.x,light.GetTransform().position.y,light.GetTransform().position.z,0},
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

	float outsideAngle = 0.150f;
	float insideAngle = 0.450f;
	float strength = 1.0f;
	float specularFactor = 1.0f;
	virtual LightTypeID getType() const override final { return Spotlight; }
};

template<>
inline hlsl_GenericLight convertLightTo_HLSL<SpotLight>(const SpotLight& light)
{
	return hlsl_GenericLight{
		.direction = light.GetTransform().rotation,
		.range = {light.insideAngle,light.insideAngle,light.insideAngle,light.insideAngle},
		.position = vec4{light.GetTransform().position.x,light.GetTransform().position.y,light.GetTransform().position.z,0},
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

///  You should not instantiate this ! The sceneActors, present in every scene, contains this. 
///	 Add you base lights directly to this, or use the widget. The widget will automatically fetch all the scene lights.

/// =============================================================================================================================================================///

struct LightsCollections {

	std::vector<SpotLight> Spots;
	std::vector<PointLight> Points;
	std::vector<DirectionalLight> Directionals;

	template<size_t N = 16>
	std::array<hlsl_GenericLight, N> ConvertCollectionToHLSL() const
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

	std::vector<BaseLight*> toBaseLights() const
	{
		std::vector<BaseLight*> res;
		for (auto& p : Points) res.push_back((BaseLight*)&p);
		for (auto& d : Directionals) res.push_back((BaseLight*)&d);
		for (auto& l : Spots) res.push_back((BaseLight*)&l);
		return res;
	}

	void AddLight(const BaseLight* light)
	{
		switch (light->getType())
		{
		case LightTypeID::Point: Points.push_back(*reinterpret_cast<const PointLight*>(light)); break;
		case LightTypeID::Directional: Directionals.push_back(*reinterpret_cast<const DirectionalLight*>(light)); break;
		case LightTypeID::Spotlight: Spots.push_back(*reinterpret_cast<const SpotLight*>(light)); break;
		}
	}

	void Clear()
	{
		Spots.clear(), Directionals.clear(); Points.clear();
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