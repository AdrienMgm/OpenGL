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
	/// Simple Red
	// FragColor = vec4(0.5, 0.1, 0.1, 1.);

	/// Normal
	//FragColor = In.Normal;

	/// Position
	//FragColor = vec4(vec3(In.Position), 0);
	
	/// TexCoord
	// FragColor = vec4(vec2(In.TexCoord), 0, 1);

	/// Procedural texture
	// float tex = smoothstep(0.3, 0.32, length(fract(5.0*In.TexCoord)-0.5)) * In.Position.x;

	// FragColor = vec4(0.8, 0.2, 0.3, 1) - tex;

	/// Texture mix + Illumination
	// vec3 Light = vec3(0, 4, 20);

	// vec3 diffuse = texture(Diffuse, In.TexCoord).rgb;
	// vec3 speculaire = texture(Speculaire, In.TexCoord).rgb;
	// // vec3 diffuseColor = mix(diffuse, speculaire, 0.5);

	// vec3 l = normalize(Light - In.Position);

	// float ndotl =  clamp(dot(In.Normal, l), 0.0, 1.0);
	// vec3 color = diffuse * ndotl;
	
	// FragColor = vec4(color, 1);

	/// Tex + specular
	vec3 Light = vec3(0, 4, 20);
	float specularPower = 10;

	vec3 diffuse = texture(Diffuse, In.TexCoord).rgb;
	vec3 speculaire = texture(Speculaire, In.TexCoord).rgb;

	vec3 v = normalize(CameraPosition - In.Position);
	vec3 l = normalize(Light - In.Position);
	vec3 h = normalize(l + v);
	float ndoth =  clamp(dot(In.Normal, h), 0.0, 1.0);
	vec3 specularColor =  speculaire * pow(ndoth, specularPower);

	vec3 finalColor = mix(diffuse, specularColor, 0.5);

	FragColor = vec4(finalColor, 1);
}
