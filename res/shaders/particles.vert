#version 430 core
#extension GL_ARB_shader_storage_buffer_object : enable


struct particle{
    vec3 position;
    uint emitter;
    vec3 scale;
    uint emitter_prototype;
    vec4 rotation;

    vec3 velocity;
    int p1;

    float life;
    uint next;
    uint prev;
    float p;
};
struct emitter_prototype{
    float emission_rate;
    float lifetime;
    float rotation_rate;
    int id;
    vec4 color;
    vec3 velocity;
    int maxCount;
};
struct emitter{
    uint transform;
    uint emitter_prototype;
    float emission;
    int count;
};

mat4 translate(mat4 m, vec3 translation){
		mat4 t = {{1,0,0,translation.x},
		{0,1,0,translation.y,},
		{0,0,1,translation.z}
		,{0,0,0,1}};
		return m * transpose(t);
}
mat4 scale(mat4 m, vec3 scale){
	mat4 s = {{scale.x,0,0,0}, {0,scale.y,0,0},{0,0,scale.z,0},{0,0,0,1}};
	return m * transpose(s);
}

mat4 rotate(mat4 m, vec4 q){
	mat4 r1 = {{q.w,q.z,-q.y,q.x},{-q.z,q.w,q.x,q.y},{q.y,-q.x,q.w,q.z},{-q.x,-q.y,-q.z,q.w}};
	mat4 r2 = {{q.w,q.z,-q.y,-q.x},{-q.z,q.w,q.x,-q.y},{q.y,-q.x,q.w,-q.z},{q.x,q.y,q.z,q.w}};
	return m * r1 * r2;
}

mat4 identity(){
	mat4 i = {{1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1}};
	return i;
}

layout(std430,binding = 2) buffer p{particle particles[];};
layout(std430,binding = 3) buffer ep{emitter_prototype prototypes[];};
layout(std430,binding = 4) buffer e{emitter emitters[];};
layout(std430,binding = 5) buffer l{uint live[];};
layout(std430,binding = 1) buffer ac{uint atomicCounters[];};

// layout (location = 0) in uint index;
// layout (location = 1) in vec2 texCoord;
// layout (location = 2) in vec3 normal;
out VS_OUT{
	uint index;
}vs_out;

void main()
{
	// atomicAdd(atomicCounters[2],1);
	vs_out.index = gl_VertexID;
	// Normal = mat3(normalMat) * normal;
}