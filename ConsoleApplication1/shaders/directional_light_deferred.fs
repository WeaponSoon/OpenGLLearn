#version 330 core

in vec2 uv;

out vec4 FragColor;

uniform vec3 DirectionalLightDir;
uniform vec3 DirectionalLightColor;

uniform highp vec3 cameraPos;



uniform sampler2D gEmissiveAO;
uniform sampler2D gWorldPosMetallic;
uniform sampler2D gAlbedoSpec;
uniform sampler2D gWorldNormalRoughness;

uniform sampler2D shadowMapCSM;
uniform int numOfCSM;
uniform mat4 worldToShadowViewProj[4];
// uniform float shadowMapSize;

const float PI = 3.14159265359;
const float InvPI = 0.3183098862;

vec3 GetAlbedo(vec2 InUV)
{
	return texture(gAlbedoSpec, InUV).rgb;
}

vec3 GetSpecular(vec2 InUV)
{
	return texture(gAlbedoSpec, InUV).aaa;
}

float GetRoughness(vec2 InUV)
{
	return texture(gWorldNormalRoughness, InUV).a;
}

float GetMetallic(vec2 InUV)
{
	return texture(gWorldPosMetallic, InUV).a;
}

vec3 GetAO(vec2 InUV)
{
	return texture(gEmissiveAO, InUV).aaa;
}

vec3 GetEmissive(vec2 InUV)
{
    return texture(gEmissiveAO, InUV).rgb;
}

highp vec3 GetWorldPosition(vec2 InUV)
{
    return texture(gWorldPosMetallic, InUV).xyz;
}

vec3 GetWorldNormal(vec2 InUV)
{
    return texture(gWorldNormalRoughness, InUV).xyz;
}

float CalcNDF(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float SqNdotH = NdotH*NdotH;
    float nom   = a2;
    float denom = (SqNdotH * a2 - SqNdotH) + 1.0;
	denom = PI * denom * denom;
    return nom / denom;
}

float CalcGeometryOcculicion(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) * 0.125;
    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}

float CalcGeometryOcculicionBothDirection(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = CalcGeometryOcculicion(NdotV, roughness);
    float ggx1 = CalcGeometryOcculicion(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 CalcFreshnel(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
	vec3 albedo = GetAlbedo(uv);
	float roughness = GetRoughness(uv);
	float metallic = GetMetallic(uv);
	vec3 ao = GetAO(uv);
	vec3 specular = GetSpecular(uv);
    highp vec3 WorldPosition = GetWorldPosition(uv);
    vec3 WorldNormal = GetWorldNormal(uv);

	highp vec3 viewVector = (cameraPos - WorldPosition);
	vec3 viewDir = normalize(viewVector);

	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);

	vec3 calColor;
	//directional light
	{

// 		float shadowScalar = 1.0;
// 		for(int CSMIdx = 0; CSMIdx < numOfCSM; ++CSMIdx)
// 		{
// 			vec4 shadowUV = worldToShadowViewProj[0] * vec4(WorldPosition,1.0);
// 			shadowUV = shadowUV / shadowUV.w;
// 			vec2 csmUV = shadowUV.xy * 0.5 + vec2(0.5,0.5);
// 			if(csmUV.x > 0.0 && csmUV.x < 1.0
// 				&& csmUV.y > 0.0 && csmUV.y < 1.0)
// 			{
// 				float fCsmIdx = float(0);
// 				float fNumOfCSM = float(numOfCSM);
// 				vec2 resoveCSMUV = csmUV * vec2(1.0, 1.0/fNumOfCSM) + vec2(0.0, fCsmIdx * 1.0/fNumOfCSM);
// 				shadowScalar = shadowUV.z * 0.5 + 0.47f < texture(shadowMapCSM, resoveCSMUV).r ? 1.0 : 0.0;
// 			}
// 		}

		float shadowScalar = 1.0;
		
		
		for(int CSMIdx = 0; CSMIdx < numOfCSM; ++CSMIdx)
		{
			vec4 shadowUV = worldToShadowViewProj[CSMIdx] * vec4(WorldPosition,1.0);
			shadowUV = shadowUV / shadowUV.w;
			vec2 csmUV_base = shadowUV.xy * 0.5 + vec2(0.5,0.5);
			vec2 csmUV = csmUV_base;
   
// 			float blur_step_x = 1.0 / shadowMapSize;
// 			float blur_step_y = 1.0 / shadowMapSize;
			float blur_step_x = 1.0 / 1024.0;
			float blur_step_y = 1.0 / 1024.0;
			
			float blur_times = 0;
			int blur_size = 2;
			float avg_depth = 0;
			
			for(int x = -1 * blur_size; x <= blur_size; x++)
			{
				for(int y = -1 * blur_size; y <= blur_size; y++)
				{
					csmUV = csmUV_base + vec2(x * blur_step_x, y * blur_step_y);
					if(csmUV.x > 0.1 && csmUV.x < 0.9
						&& csmUV.y > 0.1 && csmUV.y < 0.9 && shadowUV.z < 0.9)
					{
						float fCsmIdx = float(CSMIdx);
						float fNumOfCSM = float(numOfCSM);
						vec2 resoveCSMUV = csmUV * vec2(1.0, 1.0/fNumOfCSM) + vec2(0.0, fCsmIdx * 1.0/fNumOfCSM);
						avg_depth += shadowUV.z * 0.5 + 0.47f < texture(shadowMapCSM, resoveCSMUV).r ? 1.0 : 0.0;
						blur_times++;
					}
				}
			}
			avg_depth /= blur_times;
			shadowScalar = avg_depth;
// 			shadowScalar = shadowUV.z * 0.5 + 0.47f < avg_depth ? 1.0 : 0.0;
		}	
// 		shadowScalar = shadowScalar / blur_times > 0.5? 1.0 : 0.0;
// 		shadowScalar = shadowScalar / blur_times
  
		

		//in comming
		vec3 L = DirectionalLightDir;
		vec3 H = normalize(viewDir + L);
		vec3 LC = DirectionalLightColor;

		float D = CalcNDF(WorldNormal, H, roughness);
		float G = CalcGeometryOcculicionBothDirection(WorldNormal, viewDir, L, roughness);
		vec3 F = CalcFreshnel(clamp(dot(WorldNormal, viewDir), 0.0, 1.0), F0);

		vec3 upPart = D * G * F;
		float downPart = 4*max(dot(WorldNormal,viewDir),0.0)*max(dot(WorldNormal,L),0.0) + 0.01;
		vec3 specularPart = upPart / downPart;

		vec3 kS = F;
		vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

		calColor = (kD * albedo * InvPI + specularPart) * LC * min(max(dot(WorldNormal, L),0.0), shadowScalar);
// 		calColor = texture(shadowMapCSM, resoveCSMUV).rgb;
	}

	

	vec3 finalColor = (calColor);// * specular + EnvLightColor * albedo * ao);
	FragColor = vec4(finalColor, 1);// pow(vec4(finalColor/(finalColor + vec3(1)), 1), vec4(1.0/2.2));
}