#version 430 core

#include "transform.glsl"
#include "util.glsl"

struct pointLight{
    vec3 color;
    float constant;
    float linear;
    float quadratic;
    int transformId;
    float radius;

    vec2 p;
    float cutOff;
    float outerCutOff;
};


layout (location = 0) in vec3 position;
layout(std430,binding = 1) buffer p{pointLight pointLights[];};
layout(std430, binding = 2) buffer t{_transform transforms[];};


uniform mat4 view;
uniform mat4 proj;
uniform float FC;
uniform vec3 floatingOrigin;
out vec2 TexCoords;
flat out pointLight light;

flat out vec3 lightPos;
flat out vec3 direction;

// layout(std430, binding = 0) buffer l{ vec4 lights[];};

// vec4 logVert(vec4 in_) {
// 	in_.z = log2(max(1e-6,1.0 + in_.w))*FC - 1.0f;
// 	return in_;
// }

out float logz;
void main()
{
	uint id = gl_InstanceID;
	light = pointLights[id];
	_transform t = transforms[light.transformId];
    t.position -= floatingOrigin;
	
	lightPos = t.position;
	direction = vec3(rotate(t.rotation) * vec4(0,0,1,1));
	gl_Position = proj * vec4(t.position + position * light.radius,1);
	
	logz = 1.0 + gl_Position.w;
	gl_Position.z = (log2(max(1e-6,logz))*FC - 1.0) * gl_Position.w;
	// TexCoords = ((gl_Position.xy / gl_Position.w) + vec2(1)) / 2;
	
}
