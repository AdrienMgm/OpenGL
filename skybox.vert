#version 410 core

#define POSITION	0
#define NORMAL		1
#define TEXCOORD	2

layout(location = POSITION) in vec3 Position;
layout(location = NORMAL) in vec3 Normal;
layout(location = TEXCOORD) in vec2 TexCoord;

uniform mat4 MVP;

out block
{
	vec3 position;
	vec3 normal;
	vec2 texCoord;
} Out;

void main()
{	
	vec3 pos = Position;

	Out.position = pos;
	Out.normal = Normal;
	Out.texCoord = TexCoord;

	gl_Position = MVP * vec4(pos, 1.0);
}