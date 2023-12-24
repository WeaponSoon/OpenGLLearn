layout (location = 0) in vec3 InPos;
layout (location = 1) in vec2 InTexCoords;

out vec2 TexCoords;

void main()
{
    TexCoords = InTexCoords;
	gl_Position = vec4(InPos, 1.0);
}