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

	mutable std::vector<glm::mat4> worldToShadowProj;
	std::shared_ptr<FFrameBuffer> shadowMap;

	
	

	float lightmapDistance;
	int numOfCSM;

	std::shared_ptr<FCubeTexture> envLight;
	std::shared_ptr<FCubeTexture> envSpecLight;

	FLightRenderBatch(ELightType inType, glm::vec3 inColor, glm::vec3 inDirection, glm::vec3 inLocation, float inRadius, 
		std::shared_ptr<FFrameBuffer> inShadowMap, float inLightmapDistance, int inNumOfCSM, std::shared_ptr<FCubeTexture> InEnvLight = nullptr, std::shared_ptr<FCubeTexture> InEnvSpecLight = nullptr) :
		lightType(inType), color(inColor), direction(inDirection), location(inLocation), radius(inRadius)
		, shadowMap(inShadowMap), lightmapDistance(inLightmapDistance), numOfCSM(inNumOfCSM), envLight(InEnvLight), envSpecLight(InEnvSpecLight)
	{
		worldToShadowProj.resize(numOfCSM);
	}
};

class FLightComponent : public FSceneComponent
{
public:

	bool ShouldAdd() const override { return false; }
	glm::vec3 lightColor;

	FLightComponent() : lightColor(0)
	{
		
	}

	void Init(glm::vec3 inLightColor)
	{
		lightColor = inLightColor;
	}

	virtual void GetLightRenderBatch(std::vector<FLightRenderBatch>& outElement)
	{
		
	}
};

class FEnvLightComponent : public FLightComponent
{
public:
	static std::shared_ptr<FShader> EnvLightDeferredShader;
	static std::shared_ptr<FPrimitive> EnvLightDeferredGeo;
	FEnvLightComponent() = default;

	FCubeTextureRef originEnvLight;
	FCubeTextureRef cookedEnvLight;
	FCubeTextureRef cookedSpecPrefilterLight;
	static FTextureRef cookedSpecBrdfLight;

	void CookEnvLight();

	void Init(glm::vec3 inLightColor)
	{
		FLightComponent::Init(inLightColor);
	}

	virtual void GetLightRenderBatch(std::vector<FLightRenderBatch>& outElement) override
	{
		outElement.emplace_back(ELightType::LT_Env, lightColor, GetFowardInWorldSpace(), GetWorldLocation(), 0.0f, nullptr,100.0f,1, cookedEnvLight, cookedSpecPrefilterLight);
	}
};


class FDirectionalLightComponent : public FLightComponent
{
	int baseShadowMapSize;
	int numOfCSM;
	float lightmapDistance;
	std::shared_ptr<FFrameBuffer> shadowMap;
	int GetShadowMapWidth() const
	{
		return baseShadowMapSize * numOfCSM;
	}
public:
	friend class FDirectionalLightProxy;
	static std::shared_ptr<FShader> DirectionalLightDeferredShader;
	static std::shared_ptr<FPrimitive> DirectionalLightDeferredGeo;

	void SetLightmapDistance(float inDistance)
	{
		lightmapDistance = inDistance;
	}

	void SetShadowMapSize(int inBaseShadowMapSize, int inNumOfCSM = -1)
	{
		const int needNumOfCSM = inNumOfCSM <= 0 ? numOfCSM : inNumOfCSM;
		const int needBaseShadowMapSize = inBaseShadowMapSize <= 0 ? baseShadowMapSize : inBaseShadowMapSize;
		if(needBaseShadowMapSize != baseShadowMapSize || needNumOfCSM != numOfCSM)
		{
			baseShadowMapSize = needBaseShadowMapSize;
			numOfCSM = needNumOfCSM;
			shadowMap = std::make_shared<FFrameBuffer>(baseShadowMapSize, GetShadowMapWidth(), 0, EFrameBufferColorFormat::FCF_RGBA);
		}
	}

	FDirectionalLightComponent() :  baseShadowMapSize(2048), numOfCSM(1), lightmapDistance(10.f)
		, shadowMap(std::make_shared<FFrameBuffer>(baseShadowMapSize, GetShadowMapWidth(), 0, EFrameBufferColorFormat::FCF_RGBA))
	{
		 
	}

	void Init(glm::vec3 inLightColor, glm::quat inRotation)
	{
		FLightComponent::Init(inLightColor);
		SetWorldTransform(glm::mat4_cast(inRotation));
	}

	virtual void GetLightRenderBatch(std::vector<FLightRenderBatch>& outElement) override
	{
		outElement.emplace_back(ELightType::LT_Directional, lightColor, GetFowardInWorldSpace(), GetWorldLocation(), 0.0f, shadowMap, lightmapDistance, numOfCSM);
	}
};

class FPointLightComponent : public FLightComponent
{
public:

	static std::shared_ptr<FShader> PointLightDeferredShader;
	static std::shared_ptr<FPrimitive> PointLightDeferredGeo;

	glm::vec3 pointLightParam;

	float radius = 0;

	FPointLightComponent() = default;

	void Init(glm::vec3 inLightColor, glm::vec3 inLightLocation, glm::vec3 inPointLightParam, float inRadius)
	{
		pointLightParam = inPointLightParam;
		SetWorldLocation(inLightLocation);
		radius = inRadius;
	}

	virtual void GetLightRenderBatch(std::vector<FLightRenderBatch>& outElement) override
	{
		outElement.emplace_back(ELightType::LT_Point, lightColor, pointLightParam, GetWorldLocation(), radius, nullptr, 100.f,1);
	}

};

class FLightProxy : public FSceneProxy
{
public:

	glm::vec3 lightColor;

	FLightProxy(const FLightComponent* InComponent) :FSceneProxy(InComponent),lightColor(InComponent->lightColor)
	{

	}

	void Init(glm::vec3 inLightColor)
	{
		lightColor = inLightColor;
	}

	virtual void GetLightRenderBatch(std::vector<FLightRenderBatch>& outElement)
	{

	}
};
class FEnvLightProxy : public FLightProxy
{
public:
	std::shared_ptr<FShader> EnvLightDeferredShader;
	std::shared_ptr<FPrimitive> EnvLightDeferredGeo;
	FCubeTextureRef originEnvLight;
	FCubeTextureRef cookedEnvLight;
	FCubeTextureRef cookedSpecPrefilterLight;
	FTextureRef cookedSpecBrdfLight;
	FEnvLightProxy(const FEnvLightComponent* InComponent) :FLightProxy(InComponent),
		EnvLightDeferredShader(InComponent->EnvLightDeferredShader),
		EnvLightDeferredGeo(InComponent->EnvLightDeferredGeo),
		originEnvLight(InComponent->originEnvLight),
		cookedEnvLight(InComponent->cookedEnvLight),
		cookedSpecPrefilterLight(InComponent->cookedSpecPrefilterLight),
		cookedSpecBrdfLight(InComponent->cookedSpecBrdfLight)
	{};



	void CookEnvLight();

	virtual void GetLightRenderBatch(std::vector<FLightRenderBatch>& outElement) override
	{
		outElement.emplace_back(ELightType::LT_Env, lightColor, GetFowardInWorldSpace(), GetWorldLocation(), 0.0f, nullptr, 100.0f, 1, cookedEnvLight, cookedSpecPrefilterLight);
	}
};
class FDirectionalLightProxy : public FLightProxy
{

	int GetShadowMapWidth() const
	{
		return baseShadowMapSize * numOfCSM;
	}
public:

	int baseShadowMapSize;
	int numOfCSM;
	float lightmapDistance;
	std::shared_ptr<FFrameBuffer> shadowMap;
	std::shared_ptr<FShader> DirectionalLightDeferredShader;
	std::shared_ptr<FPrimitive> DirectionalLightDeferredGeo;

	void SetLightmapDistance(float inDistance)
	{
		lightmapDistance = inDistance;
	}

	void SetShadowMapSize(int inBaseShadowMapSize, int inNumOfCSM = -1)
	{
		const int needNumOfCSM = inNumOfCSM <= 0 ? numOfCSM : inNumOfCSM;
		const int needBaseShadowMapSize = inBaseShadowMapSize <= 0 ? baseShadowMapSize : inBaseShadowMapSize;
		if (needBaseShadowMapSize != baseShadowMapSize || needNumOfCSM != numOfCSM)
		{
			baseShadowMapSize = needBaseShadowMapSize;
			numOfCSM = needNumOfCSM;
			shadowMap = std::make_shared<FFrameBuffer>(baseShadowMapSize, GetShadowMapWidth(), 0, EFrameBufferColorFormat::FCF_RGBA);
		}
	}

	FDirectionalLightProxy(const FDirectionalLightComponent* InComponent) : FLightProxy(InComponent),baseShadowMapSize(InComponent->baseShadowMapSize), numOfCSM(InComponent->numOfCSM), lightmapDistance(InComponent->lightmapDistance)
		, shadowMap(InComponent->shadowMap)
	{

	}


	virtual void GetLightRenderBatch(std::vector<FLightRenderBatch>& outElement) override
	{
		outElement.emplace_back(ELightType::LT_Directional, lightColor, GetFowardInWorldSpace(), GetWorldLocation(), 0.0f, shadowMap, lightmapDistance, numOfCSM);
	}
};
class FPointLightLightProxy : public FLightProxy
{
public:

	std::shared_ptr<FShader> PointLightDeferredShader;
	std::shared_ptr<FPrimitive> PointLightDeferredGeo;

	glm::vec3 pointLightParam;

	float radius = 0;

	FPointLightLightProxy(const FPointLightComponent* InComponent) : FLightProxy(InComponent), PointLightDeferredShader(InComponent->PointLightDeferredShader), pointLightParam(InComponent->pointLightParam), radius(InComponent->radius)
		
	{

	}


	virtual void GetLightRenderBatch(std::vector<FLightRenderBatch>& outElement) override
	{
		outElement.emplace_back(ELightType::LT_Point, lightColor, pointLightParam, GetWorldLocation(), radius, nullptr, 100.f, 1);
	}

};