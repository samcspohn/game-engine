#version 430 core

layout (location = 0) in vec3 position;

uniform mat4 mv;
uniform mat4 proj;
uniform float FC;
out vec2 TexCoords;

// layout(std430, binding = 0) buffer l{ vec4 lights[];};

// vec4 logVert(vec4 in_) {
// 	in_.z = log2(max(1e-6,1.0 + in_.w))*FC - 1.0f;
// 	return in_;
// }

out float logz;
void main()
{
	// uint id = gl_InstanceID;

	// vec3 pos = vec3(mv * vec4(position,1));
	// if(pos.z >= 0){
	// 	pos.z = -1e-12f;
	// }
	gl_Position = proj * mv * vec4(position,1);
	// gl_Position.z = clamp(gl_Position.z, 0,-gl_Position.z);
	// gl_Position.w = clamp(gl_Position.w, 1,0);
	logz = 1.0 + gl_Position.w;
	gl_Position.z = (log2(max(1e-6,logz))*FC - 1.0) * gl_Position.w;
	// TexCoords = ((gl_Position.xy / gl_Position.w) + vec2(1)) / 2;
	
}
