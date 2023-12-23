
in vec2 uv;

/*<Switch=Deffered,Version=330>*/

#if Deffered
layout (location = 0) out vec4 gEmissiveAO;
#else
out vec4 FragColor;
#endif



uniform highp mat4 cameraModel;
uniform samplerCube evnTex;

void main()
{
    vec3 Pos = vec3(uv * vec2(2,2) - vec2(1,1), -0.5);
    vec3 Nor = normalize(Pos);

    vec3 WorldNor = (cameraModel * vec4(Nor, 0.0f)).xyz;

    #if Deffered
    gEmissiveAO = vec4(texture(evnTex, WorldNor).rgb, 1);
    #else
    FragColor = vec4(texture(evnTex, WorldNor).rgb, 1);
    #endif
}