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


out VS_OUT ts_out[];
out vec3 FragPos;
out float logz;


uniform vec3 viewDir;
uniform float FC;

layout (vertices = 3) out;


void main(){
    ts_out[gl_InvocationID] = vs_out[gl_InvocationID];

	 // Calculate the tessellation levels
    gl_TessLevelOuter[0] = 5;//GetTessLevel(EyeToVertexDistance1, EyeToVertexDistance2);
    gl_TessLevelOuter[1] = 5;//GetTessLevel(EyeToVertexDistance2, EyeToVertexDistance0);
    gl_TessLevelOuter[2] = 5;//GetTessLevel(EyeToVertexDistance0, EyeToVertexDistance1);
    gl_TessLevelInner[0] = gl_TessLevelOuter[2];

}
