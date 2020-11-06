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
    vec3 _position;
    vec2 TexCoord;
    vec2 TexCoord2;
    vec3 normal;
    uint id;
    float tessFactor;
};
in VERT vs_out[];

out VERT ts_out[];

uniform vec3 viewDir;
uniform float FC;
uniform vec3 viewPos;

layout (vertices = 3) out;
void main(){
    ts_out[gl_InvocationID] = vs_out[gl_InvocationID];
    ts_out[gl_InvocationID].position = (matrixes[ts_out[0].id].model * vec4(ts_out[gl_InvocationID].position,1)).xyz;
	 // Calculate the tessellation levels

    float tess = max(1.f, 4.f - length(ts_out[gl_InvocationID].position) / 300.f);
    ts_out[gl_InvocationID].tessFactor = max(length(ts_out[gl_InvocationID].position) / 300.f + 1, 4) * 5;
    gl_TessLevelOuter[0] = tess; 
    gl_TessLevelOuter[1] = tess;
    gl_TessLevelOuter[2] = tess;
    gl_TessLevelInner[0] = tess;
    gl_TessLevelInner[1] = tess;

}
