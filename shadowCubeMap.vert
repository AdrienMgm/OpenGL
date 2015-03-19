#version 410 core

#define POSITION	0
#define NORMAL		1
#define TEXCOORD	2

layout(location = POSITION) in vec3 Position;

out block
{
	vec3 Position;
} Out;

void main()
{	
	vec3 pos = Position;

	Out.Position = pos;

	gl_Position = vec4(pos, 1.0);
}