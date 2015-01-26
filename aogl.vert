#version 410 core

#define POSITION	0
#define NORMAL		1
#define TEXCOORD	2

precision highp float;
precision highp int;

uniform mat4 MVP;
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

	if(Object == 0){
		pos.x = Position.x * cos(Time) - Position.z * sin(Time);
		pos.y = Position.y;
		pos.z = Position.x * sin(Time) + Position.z * cos(Time);

		normal.x = Normal.x * cos(Time) - Normal.z * sin(Time);
		normal.y = Normal.y;
		normal.z = Normal.x * sin(Time) + Normal.z * cos(Time);

		// if(gl_VertexID == 0 || gl_VertexID == 1 || gl_VertexID == 2 || gl_VertexID == 3) pos.y *= 1 / cos(Time);
		// if(gl_VertexID == 4 || gl_VertexID == 5 || gl_VertexID == 6 || gl_VertexID == 7) pos.y *= 1 / cos(Time);
		// if(gl_VertexID == 8 || gl_VertexID == 9 || gl_VertexID == 10 || gl_VertexID == 11) pos.y *= 1 / cos(Time);
		// if(gl_VertexID == 12 || gl_VertexID == 13 || gl_VertexID == 14 || gl_VertexID == 15) pos.y *= 1 / cos(Time);
		// if(gl_VertexID == 16 || gl_VertexID == 17 || gl_VertexID == 18 || gl_VertexID == 19) pos.y *= 1 / cos(Time);	
		// if(gl_VertexID == 20 || gl_VertexID == 21 || gl_VertexID == 22 || gl_VertexID == 23) pos.y *= 1 / cos(Time);
		// if(gl_VertexID == 24 || gl_VertexID == 25 || gl_VertexID == 26 || gl_VertexID == 27) pos.y *= 1 / cos(Time);

		pos.x += gl_InstanceID - (10/2);
		pos.y *= 1 / cos(Time*gl_InstanceID/2);

		normal.x += gl_InstanceID - (10/2);
		normal.y *= 1 / cos(Time*gl_InstanceID/2);
	}

	Out.TexCoord = TexCoord;
	Out.Normal = normal;
	Out.Position = pos;

	gl_Position = MVP * vec4(pos, 1.0);
}