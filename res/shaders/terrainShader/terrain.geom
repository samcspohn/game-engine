#version 430 core
#extension GL_ARB_shader_storage_buffer_object : enable
#include "../transform.glsl"


layout(std430, binding = 3) buffer _m_buffs{
	matrix matrixes[];
};
layout(std430,binding = 4) buffer _ids{
	uint ids[];
};


struct VERT{
    vec3 position;
    vec2 TexCoord;
    vec2 TexCoord2;
    vec3 normal;
    uint id;
    float tessFactor;
};
in VERT tse_out[];

out vec2 TexCoord;
out vec2 TexCoord2;
out vec3 Normal;
out vec3 FragPos;
out float logz;

uniform vec3 viewDir;
uniform float FC;
uniform mat4 vp;


void createVert(uint id, vec3 pos, vec3 offset){
    // vec4 o = matrixes[id].mvp * vec4(offset,1);
    gl_Position = vp * vec4(pos + offset,1);
    // gl_Position += vec4((o * gl_Position.z * 0.0005f).xyz,0);
	logz = 1.0 + gl_Position.w;
	gl_Position.z = (log2(max(1e-6,logz))*FC - 1.0) * gl_Position.w;
	FragPos = pos + offset;
    EmitVertex();
}
void makeLine(uint matId, uint id1, uint id2){
    vec3 alignment = normalize(tse_out[id1].position - tse_out[id2].position);
    vec3 offset = normalize(cross(viewDir,alignment)) * 0.03f;
	createVert(matId, tse_out[id1].position, offset);
    createVert(matId, tse_out[id1].position, -offset);
    createVert(matId, tse_out[id2].position, offset);
    createVert(matId, tse_out[id2].position, -offset);
    EndPrimitive();
}
layout (triangles) in;
layout (triangle_strip, max_vertices=12) out;
void main(){
    // Normal = normalize(cross(tse_out[0].position - tse_out[2].position, tse_out[0].position - tse_out[1].position)
    //  + tse_out[0].normal * 3 + tse_out[1].normal + tse_out[2].normal);
    Normal = tse_out[0].normal;
    gl_Position = vp * vec4(tse_out[0].position,1);
	logz = 1.0 + gl_Position.w;
	gl_Position.z = (log2(max(1e-6,logz))*FC - 1.0) * gl_Position.w;
	FragPos = tse_out[0].position;
    TexCoord = tse_out[0].TexCoord;
    TexCoord2 = tse_out[0].TexCoord2;
    EmitVertex();


    // Normal = normalize(cross(tse_out[0].position - tse_out[2].position, tse_out[0].position - tse_out[1].position)
    //  + tse_out[0].normal + tse_out[1].normal + tse_out[2].normal * 3);
    Normal = tse_out[2].normal;
    gl_Position = vp * vec4(tse_out[2].position,1);
	logz = 1.0 + gl_Position.w;
	gl_Position.z = (log2(max(1e-6,logz))*FC - 1.0) * gl_Position.w;
	FragPos = tse_out[2].position;
    TexCoord = tse_out[2].TexCoord;
    TexCoord2 = tse_out[2].TexCoord2;
    EmitVertex();

     
    // Normal = normalize(cross(tse_out[0].position - tse_out[2].position, tse_out[0].position - tse_out[1].position)
    //  + tse_out[0].normal + tse_out[1].normal * 3 + tse_out[2].normal);
    Normal = tse_out[1].normal;
    gl_Position = vp * vec4(tse_out[1].position,1);
	logz = 1.0 + gl_Position.w;
	gl_Position.z = (log2(max(1e-6,logz))*FC - 1.0) * gl_Position.w;
	FragPos = tse_out[1].position;
    TexCoord = tse_out[1].TexCoord;
    TexCoord2 = tse_out[1].TexCoord2;
    EmitVertex();

    EndPrimitive();
    // makeLine(tse_out[0].id, 0,1);
    // makeLine(tse_out[0].id, 1,2);
    // makeLine(tse_out[0].id, 2,0);
}
