#version 430

struct particle{
    vec3 position;
    uint emitter;
    vec3 scale;
    uint emitter_prototype;
    vec4 rotation;

    vec3 velocity;
    int live;

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
    int live;
    vec3 scale;
    int p;
};
struct emitter{
    uint transform;
    uint emitter_prototype;
    float emission;
    int live;
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


uniform mat4 view;
uniform mat4 vRot;
uniform mat4 projection;
uniform float FC;
uniform float aspectRatio;
out vec3 FragPos;
out float logz;
out vec4 col;


layout (points) in;
layout (triangle_strip, max_vertices=4) out;

// in VS_OUT{
// 	uint index;
// }gs_in[];
// out vec4 VertPos;
in uint index_[];

void main(){
    uint index = index_[0];
    if(particles[index].live == 1){

        // top left
        vec3 position = particles[index].position;
        mat4 model = translate(identity(),particles[index].position) * scale(identity(),particles[index].scale) * rotate(identity(),particles[index].rotation);
        mat4 mvp = projection * vRot * view * model;
        // top left

        gl_Position = mvp * vec4(-.5f,.5f,0,1);
        logz = 1.0 + gl_Position.w;
        gl_Position.z = (log2(max(1e-6,logz))*FC - 1.0) * gl_Position.w;
        // TexCoord = texCoord;
        FragPos = vec3(model * vec4(-.5f,.5f,0,1.0f));
        col = prototypes[particles[index].emitter_prototype].color;
        EmitVertex();

        // top right
        gl_Position = mvp * vec4(.5f,.5f,0,1);
        logz = 1.0 + gl_Position.w;
        gl_Position.z = (log2(max(1e-6,logz))*FC - 1.0) * gl_Position.w;
        // TexCoord = texCoord;
        FragPos = vec3(model * vec4(.5f,.5f,0,1.0f));
        col = prototypes[particles[index].emitter_prototype].color;
        EmitVertex();

        // bottom left
        gl_Position = mvp * vec4(-.5f,-.5f,0,1);
        logz = 1.0 + gl_Position.w;
        gl_Position.z = (log2(max(1e-6,logz))*FC - 1.0) * gl_Position.w;
        // TexCoord = texCoord;
        FragPos = vec3(model * vec4(-.5f,-.5f,0,1.0f));
        col = prototypes[particles[index].emitter_prototype].color;
        EmitVertex();

        // bottom right
        gl_Position = mvp * vec4(.5f,-.5f,0,1);
        logz = 1.0 + gl_Position.w;
        gl_Position.z = (log2(max(1e-6,logz))*FC - 1.0) * gl_Position.w;
        // TexCoord = texCoord;
        FragPos = vec3(model * vec4(vec3(.5f,-.5f,0),1.0f));
        col = prototypes[particles[index].emitter_prototype].color;
        EmitVertex();
        EndPrimitive();
    }
}