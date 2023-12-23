

in highp vec3 WorldPosition;
in vec2 UV;
in vec3 WorldNormal;

uniform vec4 InputColor;
uniform sampler2D TestColor;
uniform vec3 cameraPos;


out vec4 FragColor;

void main()
{

#if Test1
    FragColor = vec4(1,0,0,1);
#else
    vec3 LightDir = normalize(vec3(0.5, 0.5, 0.5));
    vec3 ViewDir = normalize(cameraPos - WorldPosition);    
    float Intensity = dot(LightDir, ViewDir);
    vec3 baseColor = texture(TestColor, UV).xyz;

    FragColor = vec4(baseColor * Intensity, 1.0);// texture(TestColor, UV);
#endif
}
