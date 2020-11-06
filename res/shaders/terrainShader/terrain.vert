#version 430 core
#extension GL_ARB_shader_storage_buffer_object : enable
#include "../transform.glsl"

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec2 uvs2;
layout (location = 3) in vec3 normal;


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
out VERT vs_out;
uniform uint matrixOffset;

void main()
{
    vs_out.id = gl_InstanceID + matrixOffset;
    vs_out.position = position;
    vs_out.TexCoord = texCoord;
    vs_out.TexCoord2 = uvs2;
    vs_out.normal = normal;
}
