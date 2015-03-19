#version 430 core
#extension GL_ARB_shader_storage_buffer_object : require
#extension GL_NV_shadow_samplers_cube : enable

in block
{
    vec2 TexCoord;
} In;

uniform sampler2D ColorBuffer;
uniform sampler2D NormalBuffer;
uniform sampler2D DepthBuffer;

uniform mat4 InverseProj;

// uniform samplerCubeShadow ShadowCubeMap;
uniform samplerCube ShadowCubeMap;

uniform int Id;

uniform vec3 CameraPosition;

struct PointLight
{
    vec3 position;
    vec3 color;
    float intensity;
    mat4 worldToLightScreen[6];
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


float vectorToDepthValue(vec3 Vec)
{
    vec3 AbsVec = abs(Vec);
    float LocalZcomp = max(AbsVec.x, max(AbsVec.y, AbsVec.z));

    const float f = 100.0;
    const float n = 1.0;
    float NormZComp = (f+n) / (f-n) - (2*f*n)/(f-n)/LocalZcomp;
    return (NormZComp + 1.0) * 0.5;
}

vec3 illuminationPointLight(vec3 objectPosition, vec3 normal, vec3 diffuseColor, vec3 specularColor, float specularPower)
{
    // SurfaceToLight
    vec3 surfaceToLight = PointLights.Lights[Id].position - objectPosition;
    float currentFragmentDistToLight = vectorToDepthValue(surfaceToLight);

    // point light
    vec3 l = normalize(surfaceToLight);
    float ndotl = clamp(dot(normal, l), 0.0, 1.0);

    // specular
    vec3 v = normalize(CameraPosition - objectPosition);
    vec3 h = normalize(l+v);
    float ndoth = clamp(dot(normal, h), 0.0, 1.0);
    vec3 specular = specularColor * pow(ndoth, specularPower);

    // attenuation
    float distance = distance(PointLights.Lights[Id].position, objectPosition);
    float attenuation = 1.0 / (pow(distance,2)*.1 + 1.0);

    // ShadowMapping
    float bias = .001f; // bias
    float shadowDepth = textureCube(ShadowCubeMap, vec3(-surfaceToLight)).r;

    vec3 color;
        // color = vec3(abs(shadowDepth));
    if(shadowDepth + bias < currentFragmentDistToLight) // La distance est supérieur à celle de la shadowMap alors c'est qu'il y a un "bloqueur"
        color = vec3(0);
    else
        color = ((diffuseColor * ndotl * PointLights.Lights[Id].color * PointLights.Lights[Id].intensity) + (specular * PointLights.Lights[Id].intensity)) * attenuation;

    // vec3 color = ((diffuseColor * ndotl * PointLights.Lights[Id].color * PointLights.Lights[Id].intensity) + (specular * PointLights.Lights[Id].intensity)) * attenuation;

    // float shadowDepth = 1.0;
    // // vec4 shadowDepth = texture(ShadowCubeMap, vec3(lP.xy, lP.z));
    // // float test = dot(l, normalize(vec3(1.f, -1.f, 0.f)));
    // if (true)
    // {
    //     // Echantillonnage de Poisson
    //     shadowDepth = 0.0;
    //     const int SampleCount = 4;
    //     const float samplesf = SampleCount;
    //     const float Spread = 1750.0;

    //     for (int i=0;i<SampleCount;i++)
    //     {
    //         int index = int(samplesf*random(vec4(gl_FragCoord.xyy, i)))%SampleCount;
    //         shadowDepth += texture(ShadowCubeMap, vec4(lP.xy + poissonDisk[index]/(Spread * 1.f/distance), lP.z -0.005, 1.0)) / samplesf;
    //     }

    //     color *= shadowDepth;
    // }
    return vec3(color);
}

void main(void)
{
    // Read gbuffer values
    vec4 colorBuffer = texture(ColorBuffer, In.TexCoord).rgba;
    vec4 normalBuffer = texture(NormalBuffer, In.TexCoord).rgba;
    float depth = texture(DepthBuffer, In.TexCoord).r;

    // Unpack values stored in the gbuffer
    vec3 diffuseColor = colorBuffer.rgb;
    vec3 specularColor = colorBuffer.aaa;
    float specularPower = normalBuffer.a;
    vec3 n = normalBuffer.rgb;

    // Convert texture coordinates into screen space coordinates
    vec2 xy = In.TexCoord * 2.0 - 1.0;
    // Convert depth to -1,1 range and multiply the point by InverseProj matrix
    vec4 wP = InverseProj * vec4(xy, depth * 2.0 -1.0, 1.0);
    // Divide by w
    vec3 objPos = vec3(wP.xyz / wP.w);

    vec3 pointLight = illuminationPointLight(objPos, n, diffuseColor, specularColor, specularPower);

    Color = vec4(pointLight, 1.0);
}