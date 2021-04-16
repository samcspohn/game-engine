#version 430 core
#extension GL_ARB_shader_storage_buffer_object : enable

// out VS_OUT{
// 	uint index;
// }vs_out;

out uint index_;

void main()
{
	// atomicAdd(atomicCounters[2],1);
	index_ = gl_VertexID;
	// Normal = mat3(normalMat) * normal;
}