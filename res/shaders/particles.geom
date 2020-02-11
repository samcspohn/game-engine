#version 430
#include "transform.glsl"
#include "util.glsl"
#include "particle.glsl"

struct d{
    float key;
    uint id;
};
struct renderParticle{
    vec3 pos1;
    uint emitterID;
    vec3 pos2;
    uint emitterProtoID;
    vec3 scale;
    float life;
};
layout(std430,binding = 0) buffer t{_transform transforms[];};
layout(std430,binding = 6) buffer d_{d data[];};
layout(std430,binding = 2) buffer p{particle particles[];};
layout(std430,binding = 3) buffer ep{emitter_prototype prototypes[];};
layout(std430,binding = 4) buffer e{emitter emitters[];};
// layout(std430,binding = 5) buffer elp{int emitters_last_particle[];};
layout(std430,binding = 8) buffer r{renderParticle render[];};

uniform vec3 cameraPos;
uniform mat4 view;
uniform mat4 vRot;
uniform mat4 projection;
uniform float FC;
uniform float aspectRatio;
out vec3 FragPos;
out float logz;
out vec4 col;
out vec2 offset;


layout (points) in;
layout (triangle_strip, max_vertices=4) out;

// in VS_OUT{
// 	uint index;
// }gs_in[];
// out vec4 VertPos;
in uint index_[];


void createVert(vec3 point, mat4 mvp, mat4 model, renderParticle rp){
        gl_Position = mvp * vec4(point,1);
        logz = 1.0 + gl_Position.w;
        gl_Position.z = (log2(max(1e-6,logz))*FC - 1.0) * gl_Position.w;
        // TexCoord = texCoord;
        FragPos = vec3(model * vec4(point,1.0f));
        col = prototypes[rp.emitterProtoID].color;
        col.a *= rp.life;
        // logz = log2(logz) * 0.5 * FC;
        EmitVertex();

}

void main(){
    // uint index = data[index_[0]].id;
    uint index = index_[0];
    // if(particles[index].live == 1){

        // top left
        // if(prototypes[particles[index].emitter_prototype].trail == 0){
            // vec3 position = particles[index].position;
            // mat4 model = translate(identity(),particles[index].position) * scale(identity(),particles[index].scale) * rotate(identity(),particles[index].rotation);
            // mat4 mvp = projection * vRot * view * model;
            // renderParticle rp;
            // rp.life = particles[index].life;
            // rp.emitterProtoID = particles[index].emitter_prototype;
            // createVert(vec3(-.5f,.5f,0),mvp,model,rp);
            // offset = gl_Position.xy;
            // createVert(vec3(.5f,.5f,0),mvp,model,rp);
            // createVert(vec3(-.5f,-.5f,0),mvp,model,rp);
            // createVert(vec3(.5f,-.5f,0),mvp,model,rp);
        // }else{
            mat4 vp = projection * vRot * view;
            renderParticle rp = render[index];
            // rp.life = 1;
            // rp.pos1 = vec3(1);
            // rp.pos2 = vec3(2);
            // rp.scale = vec3(10);
            // particle p1 = particles[index];
            // particle p2 = particles[index];
            // if(p1.next < -1 && emitters_last_particle[-p1.next - 2] == index){
            //     p2.position = transforms[emitters[p1.emitter].transform].position;
            // }else if (particles[p1.next].prev == index){
            //     p2.position = particles[p1.next].position;
            // }
            // if(length(rp.pos1 - rp.pos2) > 100)
            //     rp.pos2 = rp.pos1;
            vec3 point1 = normalize(cross(cameraPos - rp.pos1,rp.pos2 - rp.pos1)) * .5f * rp.scale.x;
            vec3 point2 = normalize(cross(cameraPos - rp.pos2,rp.pos1 - rp.pos2)) * -.5f * rp.scale.x;
            // vec3 point2 = vec3(5);
            // vec3 point1 = vec3(5);
            createVert(rp.pos1 + point1,vp,identity(),rp);
            // offset = gl_Position.xy;
            createVert(rp.pos1 - point1,vp,identity(),rp);
            createVert(rp.pos2 + point2,vp,identity(),rp);
            createVert(rp.pos2 - point2,vp,identity(),rp);
        // }
        EndPrimitive();
    // }
}