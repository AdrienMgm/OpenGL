#version 410 core

#define FRAG_COLOR	0

precision highp int;

uniform sampler2D Diffuse;
uniform sampler2D Speculaire;
uniform vec3 CameraPosition;

layout(location = FRAG_COLOR, index = 0) out vec4 FragColor;

in block
{
	vec2 TexCoord;
	vec3 Normal;
	vec3 Position;
} In;

void main()
{
	/// Blinn phong
	vec3 Light = vec3(0, 5, 5);
	float specularPower = 10;

	vec3 diffuse = texture(Diffuse, In.TexCoord).rgb;
	vec3 speculaire = texture(Speculaire, In.TexCoord).rgb;

	vec3 v = normalize(CameraPosition - In.Position);
	vec3 l = normalize(Light - In.Position);
	vec3 h = normalize(l + v);
	float ndotl =  clamp(dot(In.Normal, l), 0.0, 1.0);
	float ndoth =  clamp(dot(In.Normal, h), 0.0, 1.0);

	vec3 finalColor = (diffuse * ndotl) + (speculaire * pow(ndoth, specularPower));

	FragColor = vec4(finalColor, 1);
}
