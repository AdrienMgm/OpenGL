#version 430 core
#extension GL_ARB_shader_storage_buffer_object : require

#define FRAG_COLOR	0

precision highp int;

uniform sampler2D Diffuse;
uniform sampler2D Specular;
uniform vec3 CameraPosition;
uniform float SpecularPower;

struct PointLight
{
	vec3 position;
	vec3 color;
	float intensity;
};

struct DirectionalLight
{
	vec3 position;
	vec3 color;
	float intensity;
};

struct SpotLight
{
	vec3 position;
	float angle;
	vec3 direction;
	float penumbraAngle;
	vec3 color;
	float intensity;
};

layout(std430, binding = 0) buffer pointlights
{
	int count;
	PointLight Lights[];
} PointLights;

layout(std430, binding = 1) buffer directionallights
{
	int count;
	DirectionalLight Lights[];
} DirectionalLights;

layout(std430, binding = 2) buffer spotlights
{
	int count;
	SpotLight Lights[];
} SpotLights;

layout(location = FRAG_COLOR, index = 0) out vec4 FragColor;

in block
{
	vec2 TexCoord;
	vec3 Normal;
	vec3 Position;
} In;

vec3 pointLightIllumination(vec3 position, vec3 color, float intensity, vec3 diffuseColor)
{
    // point light
    vec3 l = normalize(position - In.Position);
    float ndotl = clamp(dot(In.Normal, l), 0.0, 1.0);

    // specular
    vec3 spec = texture(Specular, In.TexCoord).rgb;
    vec3 v = normalize(CameraPosition - In.Position);
    vec3 h = normalize(l+v);
    float ndoth = clamp(dot(In.Normal, h), 0.0, 1.0);
    vec3 specularColor =  spec * pow(ndoth, SpecularPower);

    // attenuation
    float distance = length(position - In.Position);
    float attenuation = 1.0/ (pow(distance,2)*.1 + 1.0);

    //TODO faire une specular intensity
    return ((diffuseColor * ndotl * color * intensity) + (specularColor * intensity)) * attenuation;
}

vec3 directionalLightIllumination(vec3 direction, vec3 color, float intensity, vec3 diffuseColor)
{
    // directional
    vec3 l = normalize(direction);
    float ndotl = clamp(dot(In.Normal, l), 0.0, 1.0);

    // SpecularPower
    vec3 spec = texture(Specular, In.TexCoord).rgb;
    vec3 v = normalize(CameraPosition - In.Position);
    vec3 h = normalize(l+v);
    float ndoth = clamp(dot(In.Normal, h), 0.0, 1.0);
    vec3 specularColor =  spec * pow(ndoth, SpecularPower);

    return (diffuseColor * ndotl * color * intensity) + specularColor * intensity;
}

vec3 spotLightIllumination(vec3 position, vec3 direction, vec3 color, float angle, float intensity, vec3 diffuseColor)
{
    // point light
    vec3 l = normalize(position - In.Position);
    float ndotl = clamp(dot(In.Normal, l), 0.0, 1.0);

    // specular
    vec3 spec = texture(Specular, In.TexCoord).rgb;
    vec3 v = normalize(CameraPosition - In.Position);
    vec3 h = normalize(l+v);
    float ndoth = clamp(dot(In.Normal, h), 0.0, 1.0);
    vec3 specularColor =  spec * pow(ndoth, SpecularPower);

    // attenuation
    float distance = length(position - In.Position);
    float attenuation = 1.0/ (pow(distance,2)*.1 + 1.0);

    angle = radians(angle);
    float theta = acos(dot(-l, normalize(direction)));

    float falloff = clamp(pow(((cos(theta) - cos(angle)) / (cos(angle) - (cos(0.3+angle)))), 4), 0, 1);


    return ((diffuseColor * ndotl * color * intensity) + (specularColor * intensity)) * attenuation * falloff;
}

void main()
{
	vec3 diffuse = texture(Diffuse, In.TexCoord).rgb;
	vec3 speculaire = texture(Specular, In.TexCoord).rgb;

    vec3 PointLight_1 = pointLightIllumination(PointLights.Lights[0].position, PointLights.Lights[0].color, PointLights.Lights[0].intensity, diffuse);
    vec3 PointLight_2 = pointLightIllumination(PointLights.Lights[1].position, PointLights.Lights[1].color, PointLights.Lights[1].intensity, diffuse);
	vec3 DirectionalLight_1 = directionalLightIllumination(DirectionalLights.Lights[0].position, DirectionalLights.Lights[0].color, DirectionalLights.Lights[0].intensity, diffuse);
	vec3 SpotLight_1 = spotLightIllumination(SpotLights.Lights[0].position, SpotLights.Lights[0].direction, SpotLights.Lights[0].color, SpotLights.Lights[0].angle, SpotLights.Lights[0].intensity, diffuse);

	vec3 finalColor = PointLight_1 + PointLight_2 + DirectionalLight_1 + SpotLight_1;

	FragColor = vec4(finalColor, 1);
}
