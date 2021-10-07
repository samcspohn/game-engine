#version 430
#include "../transform.glsl"
#include "../util.glsl"
#include "particle.glsl"



layout(std430,binding = 0) buffer t{_transform transforms[];};
layout(std430,binding = 6) buffer d_{d data[];};
layout(std430,binding = 2) buffer p{particle particles[];};
layout(std430,binding = 3) buffer ep{emitter_prototype prototypes[];};
layout(std430,binding = 4) buffer e{emitter emitters[];};
// layout(std430,binding = 5) buffer elp{int emitters_last_particle[];};
// layout(std430,binding = 8) buffer r{renderParticle render[];};

uniform vec3 cameraPos;
uniform mat4 view;
uniform mat4 vRot;
uniform mat3 camInv;
uniform mat4 projection;
uniform float FC;
uniform float aspectRatio;

flat out uint id;

out vec3 FragPos;
out float logz;
out vec2 uv;
out vec2 offset;
out float _life_;



layout (points) in;
layout (triangle_strip, max_vertices=4) out;

in uint index_[];


void createVert(vec3 point, mat4 mvp, mat4 model, inout d rp, float _life, vec2 _uv){
        gl_Position = mvp * vec4(point,1);
        logz = 1.0 + gl_Position.w;
        gl_Position.z = (log2(max(1e-6,logz))*FC - 1.0) * gl_Position.w;
        // TexCoord = texCoord;
        FragPos = vec3(model * vec4(point,1.0f));
        // protoID(rp);
        // col = prototypes[protoID(rp)].color;
        // col.a *= _life;
        _life_ = _life;
        uv = _uv;

        // col.a = 1;
        // logz = log2(logz) * 0.5 * FC;
        EmitVertex();

}

void main(){
    // uint index = data[index_[0]].id;
    uint index = index_[0];
    d rp = data[index];

    uint proto_id = protoID(rp);
    id = proto_id;
    float life1 = life(rp);
    float life2 = life1;
    // inverse(camInv) * 
    // smvec3 pos = ;
    vec3 position = inverse(camInv) * get(getPos(rp)) + cameraPos;
    vec2 s = getScale(rp);
    vec3 _scale = vec3(s.x,s.y,0) * prototypes[proto_id].sizeLife[int((1.f - min(max(life1,0.01f),1.f)) * 100.f)];
    vec4 rotation = get(rp.rot);

    mat4 rot = rotate(rotation);
    mat4 model = translate(position) * rot * scale(_scale);
    mat4 mvp = projection * vRot * view * model;

    if(prototypes[proto_id].trail == 1)
        life1 = min(1.f, life1 + 1 / prototypes[proto_id].emission_rate / prototypes[proto_id].lifetime);

    float t = prototypes[proto_id].trail;
    createVert(vec3(-.5f,.5f,0),mvp,model,rp, life1, vec2(0,0 + (life1) * t));
    createVert(vec3(.5f,.5f,0),mvp,model,rp, life1, vec2(1 ,0 + (life1) * t));
    createVert(vec3(-.5f,-.5f,0),mvp,model,rp, life2, vec2(0 ,1 - (1 - life2) * t));
    createVert(vec3(.5f,-.5f,0),mvp,model,rp, life2, vec2(1 ,1 - (1 - life2) * t));

    EndPrimitive();

}