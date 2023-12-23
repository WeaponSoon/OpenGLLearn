
in vec2 uv;

/*<Switch=Deffered,Version=330>*/

#if Deffered
layout (location = 0) out vec4 gEmissiveAO;
#else
out vec4 FragColor;
#endif


uniform highp float Fov;
uniform highp float NearClip;
uniform highp float Aspect;
uniform highp mat4 cameraModel;
uniform samplerCube evnTex;

void main()
{

    float V = NearClip * tan(Fov * 0.5f);
    float H = V * Aspect;

    vec3 Pos = vec3(uv * vec2(2*H,2*V) - vec2(H,V), -NearClip);
    vec3 Nor = normalize(Pos);

    vec3 WorldNor = (cameraModel * vec4(Nor, 0.0f)).xyz;

    #if Deffered
    gEmissiveAO = vec4(texture(evnTex, WorldNor).rgb, 1);
    #else
    FragColor = vec4(texture(evnTex, WorldNor).rgb, 1);
    #endif
}