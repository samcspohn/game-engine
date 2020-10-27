#version 430 core
#extension GL_ARB_shader_storage_buffer_object : enable
#include "transform.glsl"


struct VS_OUT{
    vec3 position;
};
in VS_OUT vs_out[];

out VS_OUT ts_out[];

uniform vec3 viewDir;
uniform float FC;

layout (vertices = 3) out;




void main(){
    ts_out[gl_InvocationID] = vs_out[gl_InvocationID];
	 // Calculate the tessellation levels

    float tess = max(1.f, 4.f - length(ts_out[gl_InvocationID].position) / 300.f);

    gl_TessLevelOuter[0] = tess;
    gl_TessLevelOuter[1] = tess;
    gl_TessLevelOuter[2] = tess;
    gl_TessLevelInner[0] = gl_TessLevelOuter[2];

}
