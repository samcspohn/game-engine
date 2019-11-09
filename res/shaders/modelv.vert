#version 430 core
#extension GL_ARB_shader_storage_buffer_object : enable

struct v{
	vec3 pos;
	vec3 col;
};
layout (location = 1) in vec3 pos;
layout (location = 2) in vec3 color;

out float logz;
out vec4 col;

uniform float farPlane;
uniform mat4 view;
uniform mat4 vRot;
uniform mat4 projection;

//const float C = 1;
const float FC = 2.0 / log2(farPlane + 1);

vec4 logVert(vec4 in_) {
	in_.z = log2(max(1e-6,1.0 + in_.w))*FC - 1.0f;
	return in_;
}

void main()
{
//	col = vec4(0,0,1,1);
	col = vec4(color,1);
	gl_Position = projection * vRot * view * vec4(pos,1);
	logz = 1.0 + gl_Position.w;
	gl_Position.z = (log2(max(1e-6,logz))*FC - 1.0) * gl_Position.w;
}