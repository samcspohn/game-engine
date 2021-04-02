#version 430 core
#extension GL_ARB_conservative_depth : enable
layout(depth_less) out float gl_FragDepth;

#include "../util.glsl"
#include "particle.glsl"

layout(std430,binding = 3) buffer ep{emitter_prototype prototypes[];};



uniform float FC;
uniform float screenHeight;
uniform float screenWidth;
uniform sampler2D particle_tex;

flat in uint id;

in vec2 uv;
in vec3 FragPos;
in float logz;
in vec2 offset;
in float _life_;

out vec4 color;


void main()
{   
    gl_FragDepth = log2(logz) * 0.5 * FC;
    color = prototypes[id].colorLife[int((1.f - min(max(_life_,0.01f),1.f)) * 100.f)] * texture(particle_tex, uv);
    // color.a = 1;
}