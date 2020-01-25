#version 430 core
#extension GL_ARB_conservative_depth : enable

#define buffer_depth 1
layout(depth_less) out float gl_FragDepth;

uniform float FC;

uniform float screenHeight;
uniform float screenWidth;
in vec3 FragPos;
in float logz;
in vec4 col;
in vec2 offset;
out vec4 color;


void main()
{
    
    gl_FragDepth = log2(logz) * 0.5 * FC;
    color = col;
}