#version 430 core
#extension GL_ARB_shader_storage_buffer_object : require

#define FRAG_COLOR	0

precision highp int;

uniform sampler2D Diffuse;
uniform sampler2D Specular;
uniform vec3 CameraPosition;
uniform float SpecularPower;

// Write in GL_COLOR_ATTACHMENT0
layout(location = 0 ) out vec4 Color;
// Write in GL_COLOR_ATTACHMENT1
layout(location = 1) out vec4 Normal;

in block
{
	vec2 TexCoord;
	vec3 Normal;
	vec3 Position;
} In;

void main()
{
	vec3 diffuse = texture(Diffuse, In.TexCoord).rgb;
	vec3 specular = texture(Specular, In.TexCoord).rgb;

    Normal.rgb = In.Normal;
    Normal.a = SpecularPower;
    
    Color.rgb = diffuse;
    Color.a = specular.r;
}
