#version 430 core
#extension GL_ARB_shader_storage_buffer_object : require

in block
{
    vec2 Texcoord;
} In;

uniform sampler2D ColorBuffer;
uniform sampler2D NormalBuffer;
uniform sampler2D DepthBuffer;
uniform mat4 ScreenToWorld;

uniform vec3 CameraPosition;

struct DirectionalLight
{
    vec3 position;
    vec3 color;
    float intensity;
};

layout(std430, binding = 1) buffer directionallights
{
    int count;
    DirectionalLight Lights[];
} DirectionalLights;

layout(location = 0) out vec4 Color;

vec3 illuminationDirectionalLight(vec3 positionObject, vec3 diffuseColor, vec3 specularColor, float specularPower, vec3 normal)
{
    vec3 totalColor;
    for(int i = 0; i < DirectionalLights.count; ++i)
    {
        // directional
        vec3 l = normalize(DirectionalLights.Lights[i].position);
        float ndotl = clamp(dot(normal, l), 0.0, 1.0);
        // SpecularPower
        vec3 v = normalize(CameraPosition - positionObject);
        vec3 h = normalize(l+v);
        float ndoth = clamp(dot(normal, h), 0.0, 1.0);
        vec3 specular = specularColor * pow(ndoth, specularPower);
        totalColor += (diffuseColor * ndotl * DirectionalLights.Lights[i].color * DirectionalLights.Lights[i].intensity) + specular * DirectionalLights.Lights[i].intensity;
    }
    return totalColor;
}


void main(void)
{
    // Read gbuffer values
    vec4 colorBuffer = texture(ColorBuffer, In.Texcoord).rgba;
    vec4 normalBuffer = texture(NormalBuffer, In.Texcoord).rgba;
    float depth = texture(DepthBuffer, In.Texcoord).r;

    // Unpack values stored in the gbuffer
    vec3 diffuseColor = colorBuffer.rgb;
    vec3 specularColor = colorBuffer.aaa;
    float specularPower = normalBuffer.a;
    vec3 n = normalBuffer.rgb;

    // Convert texture coordinates into screen space coordinates
    vec2 xy = In.Texcoord * 2.0 - 1.0;
    // Convert depth to -1,1 range and multiply the point by ScreenToWorld matrix
    vec4 wP = vec4(xy, depth * 2.0 -1.0, 1.0) * ScreenToWorld;
    // Divide by w
    vec3 p = vec3(wP.xyz / wP.w);

    vec3 directionalLights = illuminationDirectionalLight(p, diffuseColor, specularColor, specularPower, n);

    Color = vec4(directionalLights, 1.0);
}