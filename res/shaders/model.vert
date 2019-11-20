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

//uniform float farPlane;
//uniform mat4 dirLightTransform;
uniform mat4 proj;
uniform mat4 view;
uniform mat4 vRot;

//const float C = 1;
uniform float FC;

vec4 logVert(vec4 in_) {
	in_.z = log2(max(1e-6,1.0 + in_.w))*FC - 1.0f;
	return in_;
}
mat4 translate(mat4 m, vec3 translation){
		mat4 t = {{1,0,0,translation.x},
		{0,1,0,translation.y,},
		{0,0,1,translation.z}
		,{0,0,0,1}};
		return m * transpose(t);
}
mat4 scale(mat4 m, vec3 scale){
	mat4 s = {{scale.x,0,0,0}, {0,scale.y,0,0},{0,0,scale.z,0},{0,0,0,1}};
	return m * transpose(s);
}

mat4 rotate(mat4 m, vec4 q){
	mat4 r1 = {{q.w,q.z,-q.y,q.x},{-q.z,q.w,q.x,q.y},{q.y,-q.x,q.w,q.z},{-q.x,-q.y,-q.z,q.w}};
	mat4 r2 = {{q.w,q.z,-q.y,-q.x},{-q.z,q.w,q.x,-q.y},{q.y,-q.x,q.w,-q.z},{q.x,q.y,q.z,q.w}};
	return m * r1 * r2;
}


void main()
{
	uint id = ids[gl_InstanceID];
	col = vec4(1,1,1,1);

	mat4 model = matrixes[id].model;
	mat4 normalMat = matrixes[id].normal;
	mat4 mvp = matrixes[id].mvp;

	gl_Position = mvp * vec4(position,1);
	logz = 1.0 + gl_Position.w;
	gl_Position.z = (log2(max(1e-6,logz))*FC - 1.0) * gl_Position.w;

//	DirLightSpacePos = dirLightTransform * model * vec4(position, 1.0);

	TexCoord = texCoord;
	FragPos = vec3(model * vec4(position,1.0f));
	Normal = mat3(normalMat) * normal;
//	}
}
