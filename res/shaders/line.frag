#version 430 core
#extension GL_ARB_conservative_depth : enable
layout(depth_less) out float gl_FragDepth;

uniform float FC;

in float logz;
in vec4 col;

out vec4 color;
void main()
{   
    gl_FragDepth = log2(logz) * 0.5 * FC;
    color = col;
    // color = vec4(1,0,0,1);
    // color.a = 1;
}