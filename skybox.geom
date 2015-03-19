# version 410 core

layout(std140, column_major) uniform;
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in block
{
	vec3 position;
	vec3 texCoord;
} In[];

out block
{
	vec3 position;
	vec3 texCoord;
} Out;
 
void main() {
    for(int i = 0; i < gl_in.length(); ++i) {
        gl_Position = vec4(In[i].position, 1);
        EmitVertex();
    }
    EndPrimitive();
}