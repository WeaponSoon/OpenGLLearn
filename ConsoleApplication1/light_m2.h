#pragma once
#include "scene_m2.h"

enum class ELightType
{
	LT_Directional,
	LT_Point,
	LT_Env
};

struct FLightRenderBatch
{
	ELightType lightType;
	glm::vec3 color;
	glm::vec3 direction;
	glm::vec3 location;
	float radius;

	FLightRenderBatch(ELightType inType, glm::vec3 inColor, glm::vec3 inDirection, glm::vec3 inLocation, float inRadius) : lightType(inType), color(inColor), direction(inDirection), location(inLocation), radius(inRadius)
	{

	}
};

class FLightComponent : public FSceneComponent
{
public:
	glm::vec3 lightColor;

	FLightComponent(glm::vec3 inLightColor) : lightColor(inLightColor)
	{
		
	}

	virtual void GetLightRenderBatch(std::vector<FLightRenderBatch>& outElement)
	{
		
	}
};

class FEnvLightComponent : public FLightComponent
{
public:
	FEnvLightComponent(glm::vec3 inLightColor) : FLightComponent(inLightColor)
	{
		
	}
	virtual void GetLightRenderBatch(std::vector<FLightRenderBatch>& outElement) override
	{
		outElement.emplace_back(ELightType::LT_Env, lightColor, GetFowardInWorldSpace(), GetWorldLocation(), 0.0f);
	}
};


class FDirectionalLightComponent : public FLightComponent
{
public:

	FDirectionalLightComponent(glm::vec3 inLightColor, glm::quat inRotation) : FLightComponent(inLightColor)
	{
		SetWorldTransform(glm::mat4_cast(inRotation));
	}

	virtual void GetLightRenderBatch(std::vector<FLightRenderBatch>& outElement) override
	{
		outElement.emplace_back(ELightType::LT_Directional, lightColor, GetFowardInWorldSpace(), GetWorldLocation(), 0.0f);
	}
};

class FPointLightComponent : public FLightComponent
{
public:
	float radius;

	FPointLightComponent(glm::vec3 inLightColor, glm::vec3 inLightLocation, float inRadius) : FLightComponent(inLightColor), radius(inRadius)
	{
		SetWorldLocation(inLightLocation);
	}

	virtual void GetLightRenderBatch(std::vector<FLightRenderBatch>& outElement) override
	{
		outElement.emplace_back(ELightType::LT_Point, lightColor, GetFowardInWorldSpace(), GetWorldLocation(), radius);
	}
};
