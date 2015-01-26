#version 410 core

#define FRAG_COLOR	0

precision highp int;

uniform sampler2D Diffuse;
uniform sampler2D Speculaire;
uniform vec3 CameraPosition;

uniform vec3 PointLightPosition[2];
uniform vec3 PointLightColor[2];
uniform float PointLightIntensity[2];

layout(location = FRAG_COLOR, index = 0) out vec4 FragColor;

in block
{
	vec2 TexCoord;
	vec3 Normal;
	vec3 Position;
} In;

vec3 illuminationPointLight(vec3 PointLightPosition, vec3 PointLightColor, float PointLightIntensity, vec3 diffuse)
{
	vec3 pl = normalize(PointLightPosition - In.Position);
	float d = length(PointLightPosition - In.Position);

	float ndotpl =  clamp(dot(In.Normal, pl), 0.0, 1.0);

	return diffuse * PointLightColor * ndotpl * PointLightIntensity * (1/(d * d));	
}

void main()
{
	#define exo
	#ifdef exo
	/// Blinn phong
	vec3 light = vec3(0, 10, 10);
	float specularPower = 20;

	vec3 diffuse = texture(Diffuse, In.TexCoord).rgb;
	vec3 speculaire = texture(Speculaire, In.TexCoord).rgb;

	vec3 v = normalize(CameraPosition - In.Position);
	vec3 l = normalize(light - In.Position);
	vec3 h = normalize(l + v);
	float ndotl =  clamp(dot(In.Normal, l), 0.0, 1.0);
	float ndoth =  clamp(dot(In.Normal, h), 0.0, 1.0);

	vec3 diffuseColor = diffuse * ndotl;
	vec3 specularColor = speculaire * pow(ndoth, specularPower);

	vec3 PointLight_1 = illuminationPointLight(PointLightPosition[0], PointLightColor[0], PointLightIntensity[0], diffuse);
	vec3 PointLight_2 = illuminationPointLight(PointLightPosition[1], PointLightColor[1], PointLightIntensity[1], diffuse);

	vec3 finalColor = diffuseColor + specularColor + PointLight_1 + PointLight_2;

	FragColor = vec4(finalColor, 1);
	#endif
}
