#version 430
#include "transform.glsl"
#include "util.glsl"
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


void createVert(vec3 point, mat4 mvp, mat4 model, inout d rp){
        gl_Position = mvp * vec4(point,1);
        logz = 1.0 + gl_Position.w;
        gl_Position.z = (log2(max(1e-6,logz))*FC - 1.0) * gl_Position.w;
        // TexCoord = texCoord;
        FragPos = vec3(model * vec4(point,1.0f));
        // protoID(rp);
        col = prototypes[protoID(rp)].color;
        col.a *= life(rp);
        // logz = log2(logz) * 0.5 * FC;
        EmitVertex();

}


void rotateX(inout vec3 vec, float angle){
    float y = vec.y;
    float z = vec.z;
    vec.y = y * cos(angle) - z * sin(angle);
    vec.z = y * sin(angle) + z * cos(angle);
}

void rotateY(inout vec3 vec, float angle){
    float x = vec.x;
    float z = vec.z;
    vec.x = x * cos(angle) + z * sin(angle);
    vec.z = -x * sin(angle) + z * cos(angle);
}


float getAngle(uint a){
    return float(a) / 65536 * 6.28318530718;
}
vec3 getPoint1(inout d item){
    float anglex = getAngle(getDX1(item));
    float angley = getAngle(getDY1(item));
    vec3 p = vec3(0,0, getDZ1(item));
    rotateX(p,-anglex);
    rotateY(p,angley);
    // p.z = getDZ1(item);
    return p;
}
vec3 getPoint2(inout d item){
    float anglex = getAngle(getDX2(item));
    float angley = getAngle(getDY2(item));
    vec3 p = vec3(0,0, getDZ2(item));
    rotateX(p,-anglex);
    rotateY(p,angley);
    // p.z = getDZ2(item);
    return p;
}
void main(){
    // uint index = data[index_[0]].id;
    uint index = index_[0];

            mat4 vp = projection;// * vRot * view;
            d rp = data[index];

            vec3 p1 = getPoint1(rp);
            vec3 p2 = getPoint2(rp);

            vec3 point1 = normalize(cross(-p1,p2 - p1)) * .5f * scale(rp);
            vec3 point2 = normalize(cross(-p2,p1 - p2)) * -.5f * scale(rp);
            // vec3 point2 = vec3(5);
            // vec3 point1 = vec3(5);
            createVert(p1 + point1,vp,identity(),rp);
            // offset = gl_Position.xy;
            createVert(p1 - point1,vp,identity(),rp);
            createVert(p2 + point2,vp,identity(),rp);
            createVert(p2 - point2,vp,identity(),rp);
        // }
        EndPrimitive();
    // }
}