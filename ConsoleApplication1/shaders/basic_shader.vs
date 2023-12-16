
layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;

out highp vec3 WorldPosition;
out vec3 WorldNormal;
out vec2 UV;

/*<Switch=Test1>*/


uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;


void main()
{

#if Test1
	gl_Position = vec4(Position,1.0f);
	UV = Position.xy;
#else
	WorldPosition = (model * vec4(Position, 1.0f)).xyz;
	WorldNormal = (model * vec4(Normal, 0.0f)).xyz;
	gl_Position = projection* view * model * vec4(Position, 1.0f);
	UV = Position.xy;
#endif
	
}