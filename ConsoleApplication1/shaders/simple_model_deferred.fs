

layout (location = 0) out vec4 gEmissiveAO;
layout (location = 1) out highp vec4 gWorldPosMetallic;
layout (location = 2) out vec4 gAlbedoSpec;
layout (location = 3) out vec4 gWorldNormalRoughness;

/*<Switch=USE_NORMAL_MAP,Version=330>*/

in vec2 TexCoord;
in highp vec3 WorldPosition;
in vec3 WorldNormal;
in vec3 WorldTangent;
in vec3 WorldBitangnet;

// texture samplers
uniform sampler2D Albedo;
uniform sampler2D Specular;
uniform sampler2D Roughness;
uniform sampler2D Metallic;
uniform sampler2D AO;
uniform sampler2D Emissive;
#if USE_NORMAL_MAP
uniform sampler2D NormalMap;
#endif


const float PI = 3.14159265359;
const float InvPI = 0.3183098862;

vec3 GetAlbedo(vec2 InUV)
{
	return texture(Albedo, InUV).rgb;
}

vec3 GetSpecular(vec2 InUV)
{
	return texture(Specular, InUV).rrr;
}

float GetRoughness(vec2 InUV)
{
	return texture(Roughness, InUV).r;
}

float GetMetallic(vec2 InUV)
{
	return texture(Metallic, InUV).r;
}

vec3 GetAO(vec2 InUV)
{
	return texture(AO, InUV).rrr;
}

vec3 GetEmissive(vec2 InUV)
{
    return texture(Emissive, InUV).rgb;
}

vec3 GetNormal(vec2 InUV)
{
#if USE_NORMAL_MAP
	vec3 tangentNormal = texture(NormalMap, InUV).xyz;
    mat3 TBN = mat3(WorldTangent, WorldBitangnet, WorldNormal);
    return normalize(TBN * tangentNormal);
#else
	return WorldNormal.xyz;
#endif
}

void main()
{
    gEmissiveAO = vec4(GetEmissive(TexCoord).rgb, GetAO(TexCoord).r);
    gWorldPosMetallic = vec4(WorldPosition.xyz, GetMetallic(TexCoord));
    gAlbedoSpec = vec4(GetAlbedo(TexCoord).rgb, GetSpecular(TexCoord).r);
    gWorldNormalRoughness = vec4(GetNormal(TexCoord), GetRoughness(TexCoord));
}