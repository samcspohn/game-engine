#version 430
#include "particle.glsl"
#include "transform.glsl"
#include "util.glsl"

struct d{
    float key;
    uint id;
};

layout(std430,binding = 0) buffer t{_transform transforms[];};
layout(std430,binding = 6) buffer d_{d data[];};
layout(std430,binding = 2) buffer p{particle particles[];};
layout(std430,binding = 3) buffer ep{emitter_prototype prototypes[];};
layout(std430,binding = 4) buffer e{emitter emitters[];};
layout(std430,binding = 5) buffer elp{int emitters_last_particle[];};

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


void createVert(vec3 point, mat4 mvp, mat4 model, uint index){
        gl_Position = mvp * vec4(point,1);
        logz = 1.0 + gl_Position.w;
        gl_Position.z = (log2(max(1e-6,logz))*FC - 1.0) * gl_Position.w;
        // TexCoord = texCoord;
        FragPos = vec3(model * vec4(point,1.0f));
        col = prototypes[particles[index].emitter_prototype].color;
        col.a *= particles[index].life;
        // logz = log2(logz) * 0.5 * FC;
        EmitVertex();

}

void main(){
    uint index = data[index_[0]].id;
    // uint index = index_[0];
    // if(particles[index].live == 1){

        // top left
        if(prototypes[particles[index].emitter_prototype].trail == 0){
            vec3 position = particles[index].position;
            mat4 model = translate(identity(),particles[index].position) * scale(identity(),particles[index].scale) * rotate(identity(),particles[index].rotation);
            mat4 mvp = projection * vRot * view * model;

            createVert(vec3(-.5f,.5f,0),mvp,model,index);
            offset = gl_Position.xy;
            createVert(vec3(.5f,.5f,0),mvp,model,index);
            createVert(vec3(-.5f,-.5f,0),mvp,model,index);
            createVert(vec3(.5f,-.5f,0),mvp,model,index);
        }else{
            mat4 vp = projection * vRot * view;
            particle p1 = particles[index];
            particle p2 = particles[index];
            if(p1.next < -1 && emitters_last_particle[p1.emitter] == index){
                p2.position = transforms[emitters[p1.emitter].transform].position;
            }else if (particles[p1.next].prev == index){
                p2.position = particles[p1.next].position;
            }
            vec3 point1 = normalize(cross(cameraPos - p1.position,p2.position - p1.position)) * .5f * p1.scale.x;
            vec3 point2 = normalize(cross(cameraPos - p2.position,p1.position - p2.position)) * -.5f * p1.scale.x;
            // vec3 point2 = vec3(5);
            // vec3 point1 = vec3(5);
            createVert(p1.position + point1,vp,identity(),index);
            offset = gl_Position.xy;
            createVert(p1.position - point1,vp,identity(),index);
            createVert(p2.position + point2,vp,identity(),index);
            createVert(p2.position - point2,vp,identity(),index);
        }
        EndPrimitive();
    // }
}