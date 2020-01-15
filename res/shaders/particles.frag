#version 430 core
// #extension GL_ARB_conservative_depth : enable

// layout(depth_less) out float gl_FragDepth;

uniform float FC;

in vec3 FragPos;
in float logz;
in vec4 col;
out vec4 color;


void main()
{
    // color = vec4(1,1,1,1);
    color = col;
    gl_FragDepth = log2(logz) * 0.5 * FC;
}