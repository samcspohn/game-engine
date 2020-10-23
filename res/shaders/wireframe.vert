#version 430 core
#extension GL_ARB_shader_storage_buffer_object : enable
#include "transform.glsl"

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec2 uvs2;
layout (location = 3) in vec3 normal;


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
out VS_OUT input;
flat out uint id;
uniform uint matrixOffset;

void main()
{
    id = gl_InstanceID + matrixOffset;
    input.position = position;
}
