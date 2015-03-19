#version 410 core

#define POSITION 0

layout(location = POSITION) in vec2 Position;

out block
{
    vec2 TexCoord;
} Out;

void main()
{
    Out.TexCoord = Position * 0.5 + 0.5;
    gl_Position = vec4(Position.xy, 0.0, 1.0);
}