#version 430

struct _transform {
	vec3 position; int id;
	vec3 scale; int parent;
	vec4 rotation;
	int childrenBegin;
	int childrenEnd;
	int prevSibling;
	int nextSibling;
};

struct particle{
    vec3 position;
    uint emitter;
    vec3 scale;
    uint emitter_prototype;
    vec4 rotation;

    vec3 velocity;
    int live;

    float life;
    int next;
    int prev;
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
    int billboard;

    vec3 p;
    int trail;
};
struct emitter{
    uint transform;
    uint emitter_prototype;
    float emission;
    int live;

    vec3 p;
    int last_particle;
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

layout(std430,binding = 0) buffer t{_transform transforms[];};
layout(std430,binding = 2) buffer p{particle particles[];};
layout(std430,binding = 3) buffer ep{emitter_prototype prototypes[];};
layout(std430,binding = 4) buffer e{emitter emitters[];};


uniform vec3 cameraPos;
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


void createVert(vec3 point, mat4 mvp, mat4 model){
        gl_Position = mvp * vec4(point,1);
        logz = 1.0 + gl_Position.w;
        gl_Position.z = (log2(max(1e-6,logz))*FC - 1.0) * gl_Position.w;
        // TexCoord = texCoord;
        FragPos = vec3(model * vec4(point,1.0f));
        col = prototypes[particles[index_[0]].emitter_prototype].color;
        col.a *= particles[index_[0]].life;
        // logz = log2(logz) * 0.5 * FC;
        EmitVertex();

}

void main(){
    uint index = index_[0];
    if(particles[index].live == 1){

        // top left
        if(prototypes[particles[index].emitter_prototype].trail == 0){
            vec3 position = particles[index].position;
            mat4 model = translate(identity(),particles[index].position) * scale(identity(),particles[index].scale) * rotate(identity(),particles[index].rotation);
            mat4 mvp = projection * vRot * view * model;

            createVert(vec3(-.5f,.5f,0),mvp,model);
            createVert(vec3(.5f,.5f,0),mvp,model);
            createVert(vec3(-.5f,-.5f,0),mvp,model);
            createVert(vec3(.5f,-.5f,0),mvp,model);
        }else{
            mat4 vp = projection * vRot * view;
            particle p1 = particles[index];
            particle p2;
            if(p1.next < -1){
                p2.position = transforms[emitters[p1.emitter].transform].position;
            }else{
                p2.position = particles[p1.next].position;
            }
            vec3 point1 = normalize(cross(cameraPos - p1.position,p2.position - p1.position)) * .5f;
            vec3 point2 = normalize(cross(cameraPos - p2.position,p1.position - p2.position)) * -.5f;
            // vec3 point2 = vec3(5);
            // vec3 point1 = vec3(5);
            createVert(p1.position + point1,vp,identity());
            createVert(p1.position - point1,vp,identity());
            createVert(p2.position + point2,vp,identity());
            createVert(p2.position - point2,vp,identity());
        }
        EndPrimitive();
    }
}