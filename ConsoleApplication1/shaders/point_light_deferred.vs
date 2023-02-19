#version 330 core

layout (location = 0) in vec3 Position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;



out vec4 clipPosition;

void main()
{
    clipPosition = projection * view * model * vec4(Position, 1.0f);
    gl_Position = clipPosition;// projection * view * model * vec4(Position, 1.0f);

}