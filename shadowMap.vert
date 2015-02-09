#version 410 core

#define POSITION	0
#define NORMAL		1
#define TEXCOORD	2
#define LIGHTS		3

precision highp float;
precision highp int;

uniform mat4 MVPLight;
uniform float Time;
uniform int Object;

layout(location = POSITION) in vec3 Position;
layout(location = NORMAL) in vec3 Normal;
layout(location = TEXCOORD) in vec2 TexCoord;

out gl_PerVertex
{
	vec4 gl_Position;
};

out block
{
	vec2 TexCoord;
	vec3 Normal;
	vec3 Position;
} Out;

void main()
{	
	vec3 pos = Position;
	vec3 normal = Normal;

	Out.TexCoord = TexCoord;
	Out.Normal = normal;
	Out.Position = pos;

	gl_Position = MVPLight * vec4(pos, 1.0);
}