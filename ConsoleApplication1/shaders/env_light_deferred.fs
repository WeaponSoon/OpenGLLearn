

/*<Switch=IBLEnable,Version=330>*/



in vec2 uv;

out vec4 FragColor;

#if IBLEnable
uniform highp vec3 cameraPos;
uniform samplerCube IBLLight;
uniform samplerCube IBLSpecPrefilter;
uniform sampler2D IBLSpecBRDF;
uniform int MaxLOD;

uniform sampler2D gWorldPosMetallic;
uniform sampler2D gWorldNormalRoughness;

#endif

uniform vec3 EnvLightColor;
uniform sampler2D gEmissiveAO;
uniform sampler2D gAlbedoSpec;

vec3 GetAlbedo(vec2 InUV)
{
	return texture(gAlbedoSpec, InUV).rgb;
}

vec3 GetAO(vec2 InUV)
{
	return texture(gEmissiveAO, InUV).aaa;
}

vec3 GetEmissive(vec2 InUV)
{
    return texture(gEmissiveAO, InUV).rgb;
}

#if IBLEnable
vec3 CalcFreshnel(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
vec3 CalcFresnelRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}   
vec3 GetWorldNormal(vec2 InUV)
{
    return texture(gWorldNormalRoughness, InUV).xyz;
}

highp vec3 GetWorldPosition(vec2 InUV)
{
    return texture(gWorldPosMetallic, InUV).xyz;
}
float GetMetallic(vec2 InUV)
{
	return texture(gWorldPosMetallic, InUV).a;
}
float GetRoughness(vec2 InUV)
{
	return texture(gWorldNormalRoughness, InUV).a;
}

#endif

void main()
{
#if IBLEnable
    float metallic = GetMetallic(uv);
    vec3 ao = GetAO(uv);
    vec3 albedo = GetAlbedo(uv);
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
    float roughness = GetRoughness(uv);

    vec3 WorldPos = GetWorldPosition(uv);
    vec3 N = GetWorldNormal(uv);
    vec3 V = normalize(cameraPos - WorldPos);

    vec3 kS = CalcFreshnel(max(dot(N, V), 0.0), F0);
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;	  
    vec3 irradiance = texture(IBLLight, N).rgb;
    vec3 diffuse      = irradiance * albedo;

    vec3 R = reflect(-V, N); 
    vec3 F = CalcFresnelRoughness(max(dot(N, V), 0.1), F0, roughness);
    vec3 prefilteredColor = textureLod(IBLSpecPrefilter, R,  clamp(roughness * MaxLOD,0.0f,MaxLOD-1.01)).rgb;    
    vec2 brdf  = texture(IBLSpecBRDF, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular);

    FragColor = vec4(ambient * ao + GetEmissive(uv),1);
#else
    vec3 albedo = GetAlbedo(uv);
    vec3 ao = GetAO(uv);

    FragColor = vec4(EnvLightColor * albedo * ao + GetEmissive(uv),1);
#endif
    // float gray = dot(texture(sceneColor, uv).rgb, vec3(0.3,0.59,0.11));
    // FragColor = vec4(gray,gray,gray,1);/*  */
    // FragColor = pow(vec4(texture(sceneColor, uv).rgb, 1.0), vec4(1.0/2.2));
}
