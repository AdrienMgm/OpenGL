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

vec3 pointLightIllumination(vec3 diffuseColor, vec3 specularColor)
{
    vec3 totalColor;
    for(int i = 0; i < PointLights.count; ++i)
    {
        // point light
        vec3 l = normalize(PointLights.Lights[i].position - In.Position);
        float ndotl = clamp(dot(In.Normal, l), 0.0, 1.0);

        // specular
        vec3 v = normalize(CameraPosition - In.Position);
        vec3 h = normalize(l+v);
        float ndoth = clamp(dot(In.Normal, h), 0.0, 1.0);
        vec3 specular =  specularColor * pow(ndoth, SpecularPower);

        // attenuation
        float distance = length(PointLights.Lights[i].position - In.Position);
        float attenuation = 1.0/ (pow(distance,2)*.1 + 1.0);

        totalColor += ((diffuseColor * ndotl * PointLights.Lights[i].color * PointLights.Lights[i].intensity) + (specular * PointLights.Lights[i].intensity)) * attenuation;
    }
    return totalColor;
}

vec3 directionalLightIllumination(vec3 diffuseColor, vec3 specularColor)
{
    vec3 totalColor;
    for(int i = 0; i < DirectionalLights.count; ++i)
    {
        // directional
        vec3 l = normalize(DirectionalLights.Lights[i].position);
        float ndotl = clamp(dot(In.Normal, l), 0.0, 1.0);

        // SpecularPower
        vec3 v = normalize(CameraPosition - In.Position);
        vec3 h = normalize(l+v);
        float ndoth = clamp(dot(In.Normal, h), 0.0, 1.0);
        vec3 specular = specularColor * pow(ndoth, SpecularPower);

        totalColor += (diffuseColor * ndotl * DirectionalLights.Lights[i].color * DirectionalLights.Lights[i].intensity) + specular * DirectionalLights.Lights[i].intensity;
    }
    return totalColor;
}

vec3 spotLightIllumination(vec3 diffuseColor, vec3 specularColor)
{
    vec3 totalColor;
    for(int i = 0; i < SpotLights.count; ++i)
    {
        // point light
        vec3 l = normalize(SpotLights.Lights[i].position - In.Position);
        float ndotl = clamp(dot(In.Normal, l), 0.0, 1.0);

        // specular
        vec3 v = normalize(CameraPosition - In.Position);
        vec3 h = normalize(l+v);
        float ndoth = clamp(dot(In.Normal, h), 0.0, 1.0);
        vec3 specular =  specularColor * pow(ndoth, SpecularPower);

        // attenuation
        float distance = length(SpotLights.Lights[i].position - In.Position);
        float attenuation = 1.0/ (pow(distance,2)*.1 + 1.0);

        float angle = radians(SpotLights.Lights[i].angle);
        float theta = acos(dot(-l, normalize(SpotLights.Lights[i].direction)));

        float falloff = clamp(pow(((cos(theta) - cos(angle)) / (cos(angle) - (cos(0.3+angle)))), 4), 0, 1);

        totalColor += ((diffuseColor * ndotl * SpotLights.Lights[i].color * SpotLights.Lights[i].intensity) + (specularColor * SpotLights.Lights[i].intensity)) * attenuation * falloff;
    }
    return totalColor;
}

void main()
{
	vec3 diffuse = texture(Diffuse, In.TexCoord).rgb;
	vec3 specular = texture(Specular, In.TexCoord).rgb;

    vec3 PointLights = pointLightIllumination(diffuse, specular);
    vec3 DirectionalLights = directionalLightIllumination(diffuse, specular);
	vec3 SpotLights = spotLightIllumination(diffuse, specular);

	vec3 finalColor = PointLights + DirectionalLights + SpotLights;

	FragColor = vec4(finalColor, 1);
}
