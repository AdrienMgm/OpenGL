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

uniform sampler2DShadow ShadowMap;

uniform int Id;

uniform vec3 CameraPosition;

struct PointLight
{
    vec3 position;
    vec3 color;
    float intensity;
    mat4 worldToLightScreen;
};

layout(std430, binding = 0) buffer pointlights
{
    int count;
    PointLight Lights[];
} PointLights;

layout(location = 0) out vec4 Color;

vec2 poissonDisk[16] = vec2[](
    vec2( -0.94201624, -0.39906216 ),
    vec2( 0.94558609, -0.76890725 ),
    vec2( -0.094184101, -0.92938870 ),
    vec2( 0.34495938, 0.29387760 ),
    vec2( -0.91588581, 0.45771432 ),
    vec2( -0.81544232, -0.87912464 ),
    vec2( -0.38277543, 0.27676845 ),
    vec2( 0.97484398, 0.75648379 ),
    vec2( 0.44323325, -0.97511554 ),
    vec2( 0.53742981, -0.47373420 ),
    vec2( -0.26496911, -0.41893023 ),
    vec2( 0.79197514, 0.19090188 ),
    vec2( -0.24188840, 0.99706507 ),
    vec2( -0.81409955, 0.91437590 ),
    vec2( 0.19984126, 0.78641367 ),
    vec2( 0.14383161, -0.14100790 )
);

float random(vec4 seed)
{
    float dot_product = dot(seed, vec4(12.9898,78.233,45.164,94.673));
    return fract(sin(dot_product) * 43758.5453);
}

vec3 illuminationPointLight(vec3 positionObject, vec3 diffuseColor, vec3 specularColor, float specularPower, vec3 normal)
{
    // Light view
    vec4 wlP = PointLights.Lights[Id].worldToLightScreen * vec4(positionObject, 1.0);
    vec3 lP = vec3(wlP/wlP.w) * 0.5 + 0.5;

    // point light
    vec3 l = normalize(PointLights.Lights[Id].position - positionObject);
    float ndotl = clamp(dot(normal, l), 0.0, 1.0);

    // specular
    vec3 v = normalize(CameraPosition - positionObject);
    vec3 h = normalize(l+v);
    float ndoth = clamp(dot(normal, h), 0.0, 1.0);
    vec3 specular = specularColor * pow(ndoth, specularPower);

    // attenuation
    float distance = length(PointLights.Lights[Id].position - positionObject);
    float attenuation = 1.0 / (pow(distance,2)*.1 + 1.0);

    vec3 color = ((diffuseColor * ndotl * PointLights.Lights[Id].color * PointLights.Lights[Id].intensity) + (specular * PointLights.Lights[Id].intensity)) * attenuation;

    // ShadowMapping
    float bias = .005*tan(acos(ndotl)); // bias
    float shadowDepth = textureProj(ShadowMap, vec4(lP.xy, lP.z - bias, 1.0), 0);

    // if (any(greaterThan(color, vec3(0.001))))
    // {
    //     // Echantillonnage de Poisson
    //     float shadowDepth = 0.0;
    //     const int SampleCount = 4;
    //     const float samplesf = SampleCount;
    //     const float Spread = 1750.0;

    //     for (int i=0;i<SampleCount;i++)
    //     {
    //         int index = int(samplesf*random(vec4(gl_FragCoord.xyy, i)))%SampleCount;
    //         shadowDepth += textureProj(ShadowMap, vec4(lP.xy + poissonDisk[index]/(Spread * 1.f/distance), lP.z -0.005, 1.0), 0.0) / samplesf;
    //     }

    //     color *= shadowDepth;
    // }
    return shadowDepth * color;
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

    vec3 pointLight = illuminationPointLight(p, diffuseColor, specularColor, specularPower, n);

    Color = vec4(pointLight, 1.0);
}