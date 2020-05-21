#version 430 core
#extension GL_ARB_conservative_depth : enable
#include "util.glsl"
#include "particle.glsl"


layout(depth_less) out float gl_FragDepth;

layout(std430,binding = 3) buffer ep{emitter_prototype prototypes[];};
uniform float FC;

uniform float screenHeight;
uniform float screenWidth;
in vec3 FragPos;
in float logz;
in vec4 col;
in vec2 offset;
in float _life_;
flat in uint id;
out vec4 color;


void main()
{   
    gl_FragDepth = log2(logz) * 0.5 * FC;
    color = prototypes[id].colorLife[int((1.f - min(max(_life_,0.01f),1.f)) * 100.f)];
    // color.a = 1;
}