#version 410 core

layout(location = 0) out vec4 Color;

uniform samplerCubeShadow cubeMap;

in block
{
	vec3 position;
	vec3 normal;
	vec2 texCoord;
} In;

void main()
{
    Color = vec4(texture(cubeMap, vec4(In.position, .35f)));
}