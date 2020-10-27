#version 430 core
#extension GL_ARB_shader_storage_buffer_object : enable
#include "transform.glsl"

struct VS_OUT{
    vec3 position;
};
in VS_OUT tse_out[];

out vec3 FragPos;
out float logz;


uniform vec3 viewDir;
uniform float FC;
uniform mat4 vp;

layout (triangles) in;
layout (triangle_strip, max_vertices=12) out;

void createVert( vec3 pos, vec3 offset){
    // vec4 o = matrixes[id].mvp * vec4(offset,1);
    gl_Position = vp * vec4(pos + offset,1);
    // gl_Position += vec4((o * gl_Position.z * 0.0005f).xyz,0);
	logz = 1.0 + gl_Position.w;
	gl_Position.z = (log2(max(1e-6,logz))*FC - 1.0) * gl_Position.w;
	FragPos = pos;
    EmitVertex();
}
void makeLine( uint id1, uint id2){
    vec3 alignment = normalize(tse_out[id1].position - tse_out[id2].position);
    vec3 offset = normalize(cross(viewDir,alignment)) * 0.03f;
	createVert( tse_out[id1].position, offset);
    createVert( tse_out[id1].position, -offset);
    createVert( tse_out[id2].position, offset);
    createVert( tse_out[id2].position, -offset);
    EndPrimitive();
}

void main(){
    makeLine(0,1);
    makeLine(1,2);
    makeLine(2,0);
}
