#include "Component.h"
#include "fast_list.h"
#include <map>
#include "rendering.h"

using namespace std;
using namespace glm;

#define MAX_PARTICLES 1024 * 1024 * 16
struct particle{
    vec3 position;
    GLuint emitter;

    vec3 scale;
    GLuint emitter_prototype;

    quat rotation;

    vec3 velocity;
    GLint live;

    float life;
    GLuint next;
    GLuint prev;
    float p;
};
gpu_vector<GLuint>* rng = new gpu_vector<GLuint>();
gpu_vector<particle>* particles = new gpu_vector<particle>();
// gpu_vector<GLuint> live;
gpu_vector_proxy<GLuint>* dead = new gpu_vector_proxy<GLuint>();
gpu_vector_proxy<GLuint>* particlesToDestroy = new gpu_vector_proxy<GLuint>();
gpu_vector<GLuint>* atomicCounters = new gpu_vector<GLuint>();
enum particleCounters{
    liveParticles=0,destroyCounter=1
};
struct emitter_prototype{
    float emission_rate;
    float lifetime;
    float rotation_rate;
    int id;

    vec4 color;

    vec3 velocity;
    int active;
    vec3 scale;
    int billboard;
};

array_heap<emitter_prototype> emitter_prototypes_;
gpu_vector<emitter_prototype>* gpu_emitter_prototypes = new gpu_vector<emitter_prototype>(); 
map<string,typename array_heap<emitter_prototype>::ref> emitter_prototypes;


class emitter_prototype_{
    typename array_heap<emitter_prototype>::ref emitterPrototype;
public:
    uint getId(){
        return emitterPrototype.index;
    }
    emitter_prototype* operator->() {
        return &emitterPrototype.data();
    }
    emitter_prototype& operator*() {
			return emitterPrototype.data();
    }
    friend emitter_prototype_ createNamedEmitter(string name);
    friend emitter_prototype_ getEmitterPrototypeByName(string name);
};

emitter_prototype_ createNamedEmitter(string name){
    emitter_prototypes.insert(std::pair(name,emitter_prototypes_._new()));
    emitter_prototype_ ret;
    ret.emitterPrototype = emitter_prototypes.at(name);
    return ret;
}

emitter_prototype_ getEmitterPrototypeByName(string name){
    emitter_prototype_ ret;
    ret.emitterPrototype = emitter_prototypes.at(name);
    return ret;
}


struct emitter{
    GLuint transform;
    GLuint emitter_prototype;
    float emission;
    int live;
};
array_heap<emitter> emitters;
gpu_vector<emitter>* gpu_emitters = new gpu_vector<emitter>();

class particle_emitter : public component {
public:
    emitter_prototype_ prototype;
    typename array_heap<emitter>::ref emitter;
    COPY(particle_emitter);
    void lateUpdate(){
        emitter->emission -= (float)(int)emitter->emission;
        emitter->emission += prototype->emission_rate * Time.deltaTime;
        emitter->emitter_prototype = prototype.getId();
        // num_particles += (int)emitter->emission;
        // emitter->transform = transform->_T.index;
    }
    void onStart(){
        emitter = emitters._new();
        emitter->transform = transform->_T.index;
        emitter->live = 1;
    }
    void onDestroy(){
        emitter->live = 0;
        emitters._delete(this->emitter);
    }
    LATE_UPDATE(particle_emitter,lateUpdate);
};


void initParticles(){

    rng->ownStorage();
    *rng->storage = vector<GLuint>({123456789, 362436069, 521288629});
    rng->bufferData();

    vector<GLuint> indexes(MAX_PARTICLES);
    for(int i = 0; i < MAX_PARTICLES; ++i){
        indexes[i] = i;
    }
    dead->bufferData(indexes);

    particles->ownStorage();
    *particles->storage = vector<particle>(MAX_PARTICLES);
    particles->bufferData();

    // live.ownStorage();
    // *live.storage = vector<uint>(MAX_PARTICLES);
    // live.bufferData();

    atomicCounters->ownStorage();
    *atomicCounters->storage = vector<GLuint>(3);
    atomicCounters->bufferData();
    // glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicCounters.bufferId);

    // particles.tryRealloc();
    // live.tryRealloc();

    particlesToDestroy->tryRealloc(MAX_PARTICLES);
    gpu_emitter_prototypes->storage = &emitter_prototypes_.data;
    gpu_emitters->storage = &emitters.data;
}

Shader particleProgram("res/shaders/particleUpdate.comp");
void updateParticles(){


    gpu_emitter_prototypes->bufferData();
    gpu_emitters->bufferData();
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
// run shader update

    glUseProgram(particleProgram.Program);

    // glUniformMatrix4fv(matPView, 1, GL_FALSE, glm::value_ptr(rj.view));


    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, GPU_TRANSFORMS->bufferId);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, particles->bufferId);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, gpu_emitter_prototypes->bufferId);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, gpu_emitters->bufferId);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, rng->bufferId);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, dead->bufferId);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, atomicCounters->bufferId);

    glUniform1fv(glGetUniformLocation(particleProgram.Program,"deltaTime"),1, &Time.deltaTime);
    float t = Time.time;
    uint32 max_particles = MAX_PARTICLES;
    
    GLuint cam = glGetUniformLocation(particleProgram.Program,"cameraPosition");
    glUniform3f(cam, mainCamPos.x, mainCamPos.y, mainCamPos.z);
    glUniform3f(glGetUniformLocation(particleProgram.Program,"cameraUp"), mainCamUp.x,mainCamUp.y,mainCamUp.z);
    glUniform1fv(glGetUniformLocation(particleProgram.Program,"time"),1, &t);
    glUniform1ui(glGetUniformLocation(particleProgram.Program,"max_particles"),max_particles);
    glUniform1ui(glGetUniformLocation(particleProgram.Program,"stage"),0);
    glUniform1ui(glGetUniformLocation(particleProgram.Program,"count"),emitters.size());
    glDispatchCompute(emitters.size() / 64 + 1, 1, 1);
    // glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);

    glUniform1ui(glGetUniformLocation(particleProgram.Program,"stage"),1);
    glDispatchCompute(MAX_PARTICLES / 64 + 1, 1, 1);
    glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER,atomicCounters->bufferId);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint) * atomicCounters->size(), atomicCounters->storage->data());
    glFlush();
    //get data back -- debug info
    // glBindBuffer(GL_SHADER_STORAGE_BUFFER,rng->bufferId);
    // glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint) * rng->size(), rng->storage->data());
    // glBindBuffer(GL_SHADER_STORAGE_BUFFER,atomicCounters.bufferId);
    // glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint) * atomicCounters.size(), atomicCounters.storage->data());
    // glBindBuffer(GL_SHADER_STORAGE_BUFFER,particles.bufferId);
    // glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(particle) * particles.size(), particles.storage->data());
    // glBindBuffer(GL_SHADER_STORAGE_BUFFER,live.bufferId);
    // glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint) * live.size(), live.storage->data());
    // glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
}

class{
    GLuint VAO = 0;
public:
    _shader particleShader;
    void init(){
        particleShader = _shader("res/shaders/particles.vert", "res/shaders/particles.geom", "res/shaders/particles.frag");
        if(this->VAO == 0){
            glGenVertexArrays( 1, &this->VAO );
        }
        glBindVertexArray( this->VAO );
        // glBindBuffer(GL_ARRAY_BUFFER, dead.bufferId);
        // glEnableVertexAttribArray(0);
		// glVertexAttribIPointer(0, 1,  GL_UNSIGNED_INT, sizeof(GLuint),0);
        glBindVertexArray( 0 );
    }

    void drawParticles(mat4 view, mat4 rot, mat4 proj){

        glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

        float farplane = 1e32f;
        particleShader.s->shader->Use();
        GLuint matPView = glGetUniformLocation(particleShader.s->shader->Program, "view");
        GLuint matvRot = glGetUniformLocation(particleShader.s->shader->Program, "vRot");
        GLuint matProjection = glGetUniformLocation(particleShader.s->shader->Program, "projection");

        glUniform1f(glGetUniformLocation(particleShader.s->shader->Program, "aspectRatio"), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT);
        glUniform1f(glGetUniformLocation(particleShader.s->shader->Program, "FC"), 2.0 / log2(farplane + 1));
        glUniformMatrix4fv(matPView, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(matvRot, 1, GL_FALSE, glm::value_ptr(rot));
        glUniformMatrix4fv(matProjection, 1, GL_FALSE, glm::value_ptr(proj));

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, particles->bufferId);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, gpu_emitter_prototypes->bufferId);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, gpu_emitters->bufferId);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1 ,atomicCounters->bufferId);
        // glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5 ,live.bufferId);

        glBindVertexArray( this->VAO );

        // glBindBuffer( GL_ARRAY_BUFFER, live.bufferId);
        // glEnableVertexAttribArray(0);
		// glVertexAttribIPointer(0, 1,  GL_UNSIGNED_INT, sizeof(GLuint),0);
        // cout << atomicCounters.storage->at(particleCounters::liveParticles) << endl;
        glDrawArrays(GL_POINTS,0,MAX_PARTICLES);   

        // glBindBuffer(GL_SHADER_STORAGE_BUFFER,atomicCounters.bufferId);
        // glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint) * atomicCounters.size(), atomicCounters.storage->data());

		glBindBuffer(GL_ARRAY_BUFFER,0);
		glBindVertexArray( 0 );

    }
}particle_renderer;