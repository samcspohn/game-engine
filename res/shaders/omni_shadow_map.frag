#version 430

in vec4 FragPos;

uniform vec3 lightPos;
uniform float farPlane;

void main(){
	float _distance = length(FragPos.xyz - lightPos);
	_distance = _distance / farPlane;
	gl_FragDepth = _distance;

}