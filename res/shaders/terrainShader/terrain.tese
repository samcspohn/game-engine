#version 430 core
#extension GL_ARB_shader_storage_buffer_object : enable
#include "../transform.glsl"


struct VERT{
    vec3 position;
    vec3 _position;
    vec2 TexCoord;
    vec2 TexCoord2;
    vec3 normal;
    uint id;
    float tessFactor;
};
struct Material{
    sampler2D texture_diffuse0;
	sampler2D texture_diffuse1;
	sampler2D texture_diffuse2;
	sampler2D texture_diffuse3;
	sampler2D texture_specular0;
	sampler2D texture_normal0;
	float shininess;
};

layout(triangles, fractional_even_spacing, ccw) in;
in VERT ts_out[];
// out VERT tse_out;


out vec2 TexCoord;
out vec2 TexCoord2;
out vec3 Normal;
out vec3 FragPos;
out vec4 DirLightSpacePos;
out float logz;


uniform vec3 viewDir;
uniform float FC;
// uniform sampler2D noise;
uniform vec3 viewPos;
uniform mat4 vp;

uniform Material material;


vec3 interpolate3D(vec3 v0, vec3 v1, vec3 v2)
{
    return vec3(gl_TessCoord.x) * v0 + vec3(gl_TessCoord.y) * v1 + vec3(gl_TessCoord.z) * v2;
}
vec2 interpolate2D(vec2 v0, vec2 v1, vec2 v2)
{
    return vec2(gl_TessCoord.x) * v0 + vec2(gl_TessCoord.y) * v1 + vec2(gl_TessCoord.z) * v2;
}

float getNoise(vec2 xy){
    return texture(material.texture_diffuse0, (xy + 0.5f) / 512.f).r;
}

float getHeight(vec2 coord){
    return -viewPos.y - 4000 + (getNoise(coord / 20.f) * 300  + getNoise(coord / 5.f) * 50.f  + getNoise(coord) * 10.f + getNoise(coord * 5) * 2.f) * 20.f;
}
vec2 getCoord(vec2 zx){
    return (viewPos.zx + zx) / 20.f;
}
void main(){
    // tse_out.tessFactor = ts_out[0].tessFactor;
    vec3 position = interpolate3D(ts_out[0].position, ts_out[1].position, ts_out[2].position);
    // tse_out.normal = normalize(interpolate3D(ts_out[0].normal, ts_out[1].normal, ts_out[2].normal));
    TexCoord = interpolate2D(ts_out[0].TexCoord, ts_out[1].TexCoord, ts_out[2].TexCoord);
    TexCoord2 = interpolate2D(ts_out[0].TexCoord2, ts_out[1].TexCoord2, ts_out[2].TexCoord2);
    // vec2 coord = vec2(float(int(tse_out.position.x) % 500) / 500.f, float(int(tse_out.position.z) % 500) / 500.f);
    vec2 coord = (viewPos + position).zx / 20.f;
    position.y = getHeight(coord);
    FragPos = position;

    gl_Position = vp * vec4(position,1);
	logz = 1.0 + gl_Position.w;
	gl_Position.z = (log2(max(1e-6,logz))*FC - 1.0) * gl_Position.w;

    vec3 pos0 = position;
    float tess = ts_out[0].tessFactor;
    vec3 forw = pos0 + vec3(0,0,tess);
    forw.y = getHeight(getCoord(forw.zx));
    // forw.x = 0; forw.z = 1;
    vec3 back = pos0 + vec3(0,0,-tess);
    back.y = getHeight(getCoord(back.zx));
    // back.x = 0; back.z = 1;
    vec3 left = pos0 + vec3(tess,0,0);
    left.y = getHeight(getCoord(left.zx));
    // left.x = 1; left.z = 0;
    vec3 right = pos0 + vec3(-tess,0,0);
    right.y = getHeight(getCoord(right.zx));
    // right.x = -1; right.z = 0;

    Normal = normalize(cross(forw - pos0,left - pos0) + 
                    cross(left - pos0, back - pos0) + 
                    cross(back - pos0, right - pos0) + 
                    cross(right - pos0, forw - pos0));

    // tse_out.position.y += 10.f;
}
