#version 430 core
uniform vec3 lightColor;

in float logz;
in vec4 col;
out vec4 color;
uniform float farPlane;
const float FC = 2.0 / log2(farPlane + 1);

void main()
{
	color = col;
	gl_FragDepth = log2(logz) * 0.5 * FC;;
}