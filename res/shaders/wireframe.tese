#version 430 core
#extension GL_ARB_shader_storage_buffer_object : enable
#include "transform.glsl"


struct VS_OUT{
    vec3 position;
};

layout(triangles, equal_spacing, cw) in;
in VS_OUT ts_out[];
out VS_OUT tse_out;

uniform vec3 viewDir;
uniform float FC;

vec3 interpolate3D(vec3 v0, vec3 v1, vec3 v2)
{
    return vec3(gl_TessCoord.x) * v0 + vec3(gl_TessCoord.y) * v1 + vec3(gl_TessCoord.z) * v2;
}

void main(){
    tse_out.position = interpolate3D(ts_out[0].position, ts_out[1].position, ts_out[2].position);
}
