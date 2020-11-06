#version 430 core
#extension GL_ARB_shader_storage_buffer_object : enable
#include "transform.glsl"

//vector(matrixes, matrix, 3);
layout(std430, binding = 3) buffer _m_buffs{
	matrix matrixes[];
};
layout(std430,binding = 4) buffer _ids{
	uint ids[];
};


struct VS_OUT{
    vec3 position;
};
in VS_OUT vs_out[];
flat in uint id[];


out vec3 FragPos;
out float logz;


uniform vec3 viewDir;
uniform float FC;


layout (triangles) in;
layout (triangle_strip, max_vertices=12) out;

void createVert(uint id, vec3 pos, vec3 offset){
    // vec4 o = matrixes[id].mvp * vec4(offset,1);
    gl_Position = matrixes[id].mvp * vec4(pos + offset,1);
    // gl_Position += vec4((o * gl_Position.z * 0.0005f).xyz,0);
	logz = 1.0 + gl_Position.w;
	gl_Position.z = (log2(max(1e-6,logz))*FC - 1.0) * gl_Position.w;
	FragPos = (matrixes[id].model * vec4(pos,1.0f)).xyz;
    EmitVertex();
}
void makeLine(uint matId, uint id1, uint id2){
    vec3 alignment = normalize(vs_out[id1].position - vs_out[id2].position);
    vec3 offset = normalize(cross(viewDir,alignment)) * 0.01f;
	createVert(matId, vs_out[id1].position, offset);
    createVert(matId, vs_out[id1].position, -offset);
    createVert(matId, vs_out[id2].position, offset);
    createVert(matId, vs_out[id2].position, -offset);
    EndPrimitive();
}

void main(){
    uint _id = id[0];
    makeLine(_id,0,1);
    makeLine(_id,1,2);
    makeLine(_id,2,0);
}
