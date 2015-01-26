#version 410 core

precision highp float;
precision highp int;
layout(std140, column_major) uniform;
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in block
{
    vec2 TexCoord;
    vec3 Normal;
	vec3 Position;
} In[]; 

out block
{
    vec2 TexCoord;
    vec3 Normal;
    vec3 Position;
}Out;

uniform mat4 MVP;
uniform mat4 MV;
uniform float Time;

void main()
{   
    for(int i = 0; i < gl_in.length(); ++i)
    {
        // if(gl_PrimitiveIDIn == 0 || gl_PrimitiveIDIn == 1)
        // {
        //     Out.Normal = In[i].Normal - .2;
        //     Out.TexCoord = In[i].TexCoord;
        //     Out.Position = In[i].Position - .2;
        //     gl_Position = gl_in[i].gl_Position - .2;
        //     EmitVertex();
        // }
        // else
        {
            Out.Normal = In[i].Normal;
            Out.TexCoord = In[i].TexCoord;
            Out.Position = In[i].Position;
            gl_Position = gl_in[i].gl_Position;
            EmitVertex();
        }
    }
    EndPrimitive();
}