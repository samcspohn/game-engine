#version 430 core
#extension GL_ARB_shader_storage_buffer_object : enable

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec3 normal;

#define vector(name, type ,_binding) layout(std430,binding = _binding) buffer _{uint size; type name[];}


struct matrix{
	mat4 mvp;
	mat4 model;
	mat4 normal;
};

mat4 identity(){
	mat4 i = {{1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1}};
	return i;
}

//vector(matrixes, matrix, 3);
layout(std430, binding = 3) buffer _m_buffs{
	matrix matrixes[];
};
layout(std430,binding = 4) buffer _ids{
	uint ids[];
};


//out float logz;
out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;
out vec4 DirLightSpacePos;
out float logz;
out vec4 col;

//const float C = 1;
uniform float FC;

vec4 logVert(vec4 in_) {
	in_.z = log2(max(1e-6,1.0 + in_.w))*FC - 1.0f;
	return in_;
}


void main()
{
	uint id = gl_InstanceID;
	col = vec4(1,1,1,1);

	mat4 model = matrixes[id].model;
	mat4 normalMat = matrixes[id].normal;
	mat4 mvp = matrixes[id].mvp;

	gl_Position = mvp * vec4(position,1);
	logz = 1.0 + gl_Position.w;
	gl_Position.z = (log2(max(1e-6,logz))*FC - 1.0) * gl_Position.w;

//	DirLightSpacePos = dirLightTransform * model * vec4(position, 1.0);

	TexCoord = texCoord;
	FragPos = (model * vec4(position,1.0f)).xyz;
	Normal = mat3(normalMat) * normal;
//	}
}
