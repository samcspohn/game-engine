#version 430
#extension GL_ARB_shader_storage_buffer_object : enable

layout (location = 0) in vec3 position;

struct matrix{
	mat4 mvp;
	mat4 model;
	mat4 normal;
//	mat4 garbage;
};

layout(std430, binding = 3) buffer _m_buffs{
	matrix matrixes[];
};
layout(std430,binding = 4) buffer _ids{ uint ids[]; };



uniform mat4 dirLightTransform;
uniform float farPlane;

void main()
{
	mat4 model = matrixes[ids[gl_InstanceID]].model;
	gl_Position = dirLightTransform * model * vec4(position, 1.0);
}
