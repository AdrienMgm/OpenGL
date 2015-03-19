# version 410 core
 
layout(triangles, invocations = 1) in;
layout(triangle_strip, max_vertices = 18) out;

uniform mat4 ObjectToLightScreen[6];

in block
{
	vec3 Position;
} In[];
 
void main() {
    for(int layer = 0; layer < 6; ++layer) {
        gl_Layer = layer;
        for(int i = 0; i < gl_in.length(); ++i) {
            gl_Position = ObjectToLightScreen[layer] * vec4(In[i].Position, 1);
            EmitVertex();
        }
        EndPrimitive();
    }
}