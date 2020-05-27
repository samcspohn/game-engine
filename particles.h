#include "Component.h"
#include "fast_list.h"
#include <map>
#include <unordered_map>
#include "rendering.h"
#include <fstream>
using namespace std;
using namespace glm;

#define MAX_PARTICLES 1024 * 1024 * 12
struct particle
{
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
    float life2;
    vec3 position2;
    float p1;
    vec3 velocity2;
    float l;
};
struct emitter_prototype{
    float emission_rate;
    float lifetime;
    float rotation_rate;
    float dispersion;

    // vec4 color;
    void color(vec4 c){
        for(int i = 0; i < 100; ++i){
            colorLife[i] = c;
        }
    }
    void color(vec4 c1, vec4 c2){
        vec4 step = (c2 - c1) / 100.f;
        for(int i = 0; i < 100; ++i){
            colorLife[i] = c1 + step * (float)i;
        }
    }
    void size(float c){
        for(int i = 0; i < 100; ++i){
            sizeLife[i] = c;
        }
    }
    void size(float c1, float c2){
        float step = (c2 - c1) / 100.f;
        for(int i = 0; i < 100; ++i){
            sizeLife[i] = c1 + step * (float)i;
        }
    }

    float minSpeed;
    float maxSpeed;
    float lifetime2;
    int live;

    vec3 scale;
    int billboard;

    int velAlign;
    float radius;
    int p2;
    int trail;
    vec4 colorLife[100];
    float sizeLife[100];
};
struct colorArray{
    struct key{
        vec4 color;
        float pos;
    };
    vector<key> keys;
    colorArray& addKey(vec4 color, float position){
        key k;
        k.color = color;
        k.pos = position;
        keys.push_back(k);
        return *this;
    }
    void setColorArray(vec4 *colors){
        if(keys.size() == 0)
            return;
        key k = keys.front();
        if(keys.size() == 1){
            for(int i = 0; i < 100; ++i){
                colors[i] = k.color;
            }
        }else{
            int k1 = 0;
            int k2 = 1;
            for(int i = 0; i < keys[k1].pos * 100; ++i){
                colors[i] = keys[k1].color;
            }
            while(k2 < keys.size()){
                vec4 step = (keys[k2].color - keys[k1].color) / ((keys[k2].pos - keys[k1].pos) * 100);
                int start = keys[k1].pos * 100;
                int stop = keys[k2].pos * 100;
                int j = 0;
                for(int i = start; i < stop; i++,j++){
                    colors[i] = keys[k1].color + step * (float)j;
                }
                k1 = k2++;
            }
            for(int i = keys[k1].pos * 100; i < 100; ++i){
                colors[i] = keys[k1].color;
            }
        }
    }
};
struct floatArray{
    struct key{
        float value;
        float pos;
    };
    vector<key> keys;
    floatArray& addKey(float v, float position){
        key k;
        k.value = v;
        k.pos = position;
        keys.push_back(k);
        return *this;
    }
    void setFloatArray(float *floats){
        if(keys.size() == 0)
            return;
        key k = keys.front();
        if(keys.size() == 1){
            for(int i = 0; i < 100; ++i){
                floats[i] = k.value;
            }
        }else{
            int k1 = 0;
            int k2 = 1;
            for(int i = 0; i < keys[k1].pos * 100; ++i){
                floats[i] = keys[k1].value;
            }
            while(k2 < keys.size()){
                float step = (keys[k2].value - keys[k1].value) / ((keys[k2].pos - keys[k1].pos) * 100);
                int start = keys[k1].pos * 100;
                int stop = keys[k2].pos * 100;
                int j = 0;
                for(int i = start; i < stop; i++,j++){
                    floats[i] = keys[k1].value + step * (float)j;
                }
                k1 = k2++;
            }
            for(int i = keys[k1].pos * 100; i < 100; ++i){
                floats[i] = keys[k1].value;
            }
        }
    }
};
struct emitter
{
    uint transform;
    uint emitter_prototype;
    float emission;
    int live;

    int last;
    int frame;
};

struct emitterInit
{
    uint transformID;
    uint emitterProtoID;
    int live;
    int id;
};
struct _emission{
    vec3 position;
    uint emitter_prototype;
    vec3 direction;
    int emitterID;
    vec3 scale;
    int last;
};
struct _burst{
    vec3 position;
    uint emitter_prototype;
    vec3 direction;
    uint count;
    vec3 scale;
    int p1;
};
enum particleCounters
{
    liveParticles = 0,
    destroyCounter = 1
};
gpu_vector<particle> *particles = new gpu_vector<particle>();
gpu_vector_proxy<_emission> *emitted = new gpu_vector_proxy<_emission>();
gpu_vector_proxy<GLuint> *burstParticles = new gpu_vector_proxy<GLuint>();
gpu_vector<_burst> *gpu_particle_bursts = new gpu_vector<_burst>();
vector<_burst> particle_bursts;
// gpu_vector<GLuint> live;
gpu_vector_proxy<GLuint> *dead = new gpu_vector_proxy<GLuint>();
gpu_vector_proxy<GLuint> *particlesToDestroy = new gpu_vector_proxy<GLuint>();
gpu_vector<GLuint> *atomicCounters = new gpu_vector<GLuint>();

array_heap<emitter_prototype> emitter_prototypes_;
gpu_vector<emitter_prototype> *gpu_emitter_prototypes = new gpu_vector<emitter_prototype>();
map<string, typename array_heap<emitter_prototype>::ref> emitter_prototypes;

mutex burstLock;
class emitter_prototype_
{
    typename array_heap<emitter_prototype>::ref emitterPrototype;

public:
    uint getId()
    {
        return emitterPrototype.index;
    }
    emitter_prototype *operator->()
    {
        return &emitterPrototype.data();
    }
    emitter_prototype &operator*()
    {
        return emitterPrototype.data();
    }
    void burst(glm::vec3 pos, glm::vec3 dir, uint count){
        _burst b;
        b.direction = dir;
        b.count = count;
        b.position = pos;
        b.scale = vec3(1);
        b.emitter_prototype = getId();
        burstLock.lock();
        particle_bursts.push_back(b);
        burstLock.unlock();
    }
    void burst(glm::vec3 pos, glm::vec3 dir,glm::vec3 scale, uint count){
        _burst b;
        b.direction = dir;
        b.count = count;
        b.position = pos;
        b.scale = scale;
        b.emitter_prototype = getId();
        burstLock.lock();
        particle_bursts.push_back(b);
        burstLock.unlock();
    }
    friend emitter_prototype_ createNamedEmitter(string name);
    friend emitter_prototype_ getEmitterPrototypeByName(string name);
};

emitter_prototype_ createNamedEmitter(string name)
{
    emitter_prototypes.insert(std::pair(name, emitter_prototypes_._new()));
    emitter_prototype_ ret;
    ret.emitterPrototype = emitter_prototypes.at(name);
    return ret;
}

emitter_prototype_ getEmitterPrototypeByName(string name)
{
    emitter_prototype_ ret;
    ret.emitterPrototype = emitter_prototypes.at(name);
    return ret;
}

array_heap<emitter> emitters;
gpu_vector_proxy<emitter> *gpu_emitters = new gpu_vector_proxy<emitter>();
gpu_vector_proxy<emitterInit> *gpu_emitter_inits = new gpu_vector_proxy<emitterInit>();
vector<emitterInit> emitterInits;
vector<emitterInit> emitterInitsdb;
// vector<emitterInit> emitter_inits_db;

unordered_map<uint, emitterInit> emitter_inits;
bool updateEmitters = true;


class particle_emitter : public component
{
    emitter_prototype_ prototype;
    static mutex lock;

public:
    typename array_heap<emitter>::ref emitter;
    typename array_heap<GLint>::ref emitter_last_particle;
    COPY(particle_emitter);
    void setPrototype(emitter_prototype_ ep)
    {
        prototype = ep;
        emitter->emitter_prototype = ep.getId();
        updateEmitters = true;

        emitterInit ei;
        ei.emitterProtoID = prototype.getId();
        ei.live = 1;
        ei.transformID = transform->_T.index;
        ei.id = this->emitter.index;
        lock.lock();
        emitter_inits[ei.id] = ei;
        lock.unlock();
    }

    void onStart()
    {
        this->emitter = emitters._new();
        this->emitter->transform = transform->_T.index;
        this->emitter->live = 1;
        this->emitter->emission = 1;
        emitter->emitter_prototype = prototype.getId();
        emitter->frame = 0;
        updateEmitters = true;

        emitterInit ei;
        ei.emitterProtoID = prototype.getId();
        ei.live = 1;
        ei.transformID = transform->_T.index;
        ei.id = this->emitter.index;
        lock.lock();
        emitter_inits[ei.id] = ei;
        lock.unlock();

    }
    void onDestroy()
    {
        this->emitter->live = 0;
        this->emitter->emission = 0;
        emitters._delete(this->emitter);
        updateEmitters = true;

        emitterInit ei;
        ei.emitterProtoID = prototype.getId();
        ei.live = 0;
        ei.transformID = transform->_T.index;
        ei.id = this->emitter.index;
        lock.lock();
        emitter_inits[ei.id] = ei;
        lock.unlock();
    }

};
mutex particle_emitter::lock;
void initParticles()
{
    cout << "initializing particles" << endl;
    vector<GLuint> indexes(MAX_PARTICLES);
    int step = MAX_PARTICLES / 1000;
    int prog = 0;
    for (int i = 0; i < MAX_PARTICLES; ++i)
    {
        indexes[i] = i;
        if(i % step == 0){
            cout << "\r" << (float)prog++ / 10;
        }
    }
    dead->bufferData(indexes);

    particles->ownStorage();
    *particles->storage = vector<particle>(MAX_PARTICLES);
    particles->bufferData();

    emitted->tryRealloc(MAX_PARTICLES);
    burstParticles->tryRealloc(MAX_PARTICLES);

    // live.ownStorage();
    // *live.storage = vector<uint>(MAX_PARTICLES);
    // live.bufferData();

    atomicCounters->ownStorage();
    *atomicCounters->storage = vector<GLuint>(3);
    atomicCounters->bufferData();

    gpu_particle_bursts->ownStorage();
    particlesToDestroy->tryRealloc(MAX_PARTICLES);
    gpu_emitter_prototypes->storage = &emitter_prototypes_.data;
    gpu_emitters->tryRealloc(1024 * 1024 * 4);
}

Shader particleSortProgram("res/shaders/particle_sort.comp");
// Shader particleSortProgram2("res/shaders/particle_sort2.comp");
Shader particleSortProgram2("res/shaders/particleUpdate.comp");
Shader particleProgram("res/shaders/particleUpdate.comp");
int particleCount;
int actualParticles;
mutex pcMutex;
void updateParticles(vec3 floatingOrigin, uint emitterInitCount)
{
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    // run shader update

    glUseProgram(particleProgram.Program);

    // glUniformMatrix4fv(matPView, 1, GL_FALSE, glm::value_ptr(rj.view));
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, GPU_TRANSFORMS->bufferId);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, particles->bufferId);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, gpu_emitter_prototypes->bufferId);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, gpu_emitters->bufferId);
    emitted->bindData(7);
    gpu_emitter_inits->bindData(8);
    gpu_particle_bursts->bindData(9);
    burstParticles->bindData(5);
    // glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, rng->bufferId);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, dead->bufferId);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, atomicCounters->bufferId);

    float t = Time.time;
    int32 max_particles = MAX_PARTICLES;
    glUniform1fv(glGetUniformLocation(particleProgram.Program, "time"), 1, &t);
    glUniform1fv(glGetUniformLocation(particleProgram.Program, "deltaTime"), 1, &Time.deltaTime);
    glUniform3f(glGetUniformLocation(particleProgram.Program, "cameraPosition"), mainCamPos.x, mainCamPos.y, mainCamPos.z);
    glUniform3f(glGetUniformLocation(particleProgram.Program, "cameraUp"), mainCamUp.x, mainCamUp.y, mainCamUp.z);
    glUniform3f(glGetUniformLocation(particleProgram.Program, "cameraForward"), MainCamForward.x, MainCamForward.y, MainCamForward.z);
    glUniform1i(glGetUniformLocation(particleProgram.Program, "max_particles"), MAX_PARTICLES);
    GLuint fo = glGetUniformLocation(particleProgram.Program, "floatingOrigin");
    glUniform3f(fo, floatingOrigin.x, floatingOrigin.y, floatingOrigin.z);

    glUniform1ui(glGetUniformLocation(particleProgram.Program, "count"), emitterInitCount);
    glUniform1ui(glGetUniformLocation(particleProgram.Program, "stage"), 2);
    glDispatchCompute(emitterInitCount / 128 + 1, 1, 1);
    glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);

    glUniform1ui(glGetUniformLocation(particleProgram.Program, "count"), emitters.size());
    glUniform1ui(glGetUniformLocation(particleProgram.Program, "stage"), 0);
    glDispatchCompute(emitters.size() / 128 + 1, 1, 1);
    glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);

    atomicCounters->retrieveData();
    (*atomicCounters)[1] = 0;
    (*atomicCounters)[2] = 0;
    atomicCounters->bufferData();
    glFlush();

    vector<uint> acs = *(atomicCounters->storage);
    
    glUniform1ui(glGetUniformLocation(particleProgram.Program, "burstOffset"), (*atomicCounters)[0]);

    glUniform1ui(glGetUniformLocation(particleProgram.Program, "count"), gpu_particle_bursts->size());
    glUniform1ui(glGetUniformLocation(particleProgram.Program, "stage"), 3);
    glDispatchCompute(gpu_particle_bursts->size() / 128 + 1, 1, 1);
    glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);

    atomicCounters->retrieveData();

    glUniform1ui(glGetUniformLocation(particleProgram.Program, "count"), (*atomicCounters)[1]);
    glUniform1ui(glGetUniformLocation(particleProgram.Program, "stage"), 4);
    glDispatchCompute((*atomicCounters)[1] / 128 + 1, 1, 1);
    glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);

    
    glUniform1ui(glGetUniformLocation(particleProgram.Program, "count"), MAX_PARTICLES);
    glUniform1ui(glGetUniformLocation(particleProgram.Program, "stage"), 1);
    glDispatchCompute(MAX_PARTICLES / 128 + 1, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

        
    glUniform1ui(glGetUniformLocation(particleProgram.Program, "count"), 1); // reset particle counter
    glUniform1ui(glGetUniformLocation(particleProgram.Program, "stage"), 6);
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);


    glUniform1ui(glGetUniformLocation(particleProgram.Program, "count"), MAX_PARTICLES); // count particles
    glUniform1ui(glGetUniformLocation(particleProgram.Program, "stage"), 5);
    glDispatchCompute(MAX_PARTICLES / 128 + 1, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    atomicCounters->retrieveData();
    pcMutex.lock();
    particleCount = atomicCounters->storage->at(0);
    actualParticles = atomicCounters->storage->at(2);
    pcMutex.unlock();
    
}

struct d
{
    // vec4 rot;
	uint xy;
    float z;
	// uint qwx;
    // uint qyz;
    uint qxy;
    uint qzw;
	uint scale_xy;
	uint protoID;
	uint key_life;
};
struct renderParticle
{
    vec3 pos1;
    uint emitterID;
    vec3 pos2;
    uint emitterProtoID;
    vec3 scale;
    float life;
};
#define COUNTER_THREADS 4096
#define BUCKETS 65536
#define buffer_depth 1

#define WG_SIZE 128
#define N_GROUPS 256
#define RADIX 12
#define BUCK (1 << RADIX)
#define BITS 32
#define BLOCK_SUM_SIZE 256
// sqrt(N_GROUPS * BUCK)
gpu_vector_proxy<uint> *counts = new gpu_vector_proxy<uint>();
gpu_vector_proxy<uint> *offsets = new gpu_vector_proxy<uint>();
gpu_vector_proxy<uint> *keys_in = new gpu_vector_proxy<uint>();
gpu_vector_proxy<uint> *keys_out = new gpu_vector_proxy<uint>();
gpu_vector_proxy<uint> *renderParticles = new gpu_vector_proxy<uint>();
gpu_vector<d> *data1 = new gpu_vector<d>();
gpu_vector_proxy<d> *data2 = new gpu_vector_proxy<d>();
gpu_vector<uint> *atomics = new gpu_vector<uint>();

gpu_vector_proxy<uint> *gpu_init = new gpu_vector_proxy<uint>();
gpu_vector_proxy<d> *gpu_init2 = new gpu_vector_proxy<d>();
gpu_vector_proxy<uint> *localCount = new gpu_vector_proxy<uint>();

vector<int> counters_(BUCKETS);
vector<int> offsets_(BUCKETS);

gpu_vector<d> *input = new gpu_vector<d>();
gpu_vector<d> *_output = new gpu_vector<d>();
gpu_vector<GLuint> *block_sums = new gpu_vector<GLuint>();
gpu_vector<GLuint> *scan = new gpu_vector<GLuint>();
gpu_vector<GLuint> *histo = new gpu_vector<GLuint>();
// gpu_vector_proxy<renderParticle> *renderParticles = new gpu_vector_proxy<renderParticle>();

// Shader renderPrepShader("res/shaders/renderParticle.comp");
bool sort1 = true;

class
{
    GLuint VAO = 0;
    
public:
    glm::mat3 camInv;
    glm::vec3 camP;

    void setCamCull(_camera* c){
        camInv = glm::mat3(c->rot);// * glm::mat3(glm::translate(-camera->transform->getPosition()));
		camP = c->pos;
    }
    _shader particleShader;
    vector<uint> zeros;

    vector<d> res;
    ofstream output;
    rolling_buffer time;
    void init()
    {
        time = rolling_buffer(1000);
        output = ofstream("particle_perf.txt", ios_base::app);

        if (zeros.size() == 0)
        {
            zeros = vector<uint>(BUCKETS);
        }

        // counts->tryRealloc(BUCKETS);
        // offsets->tryRealloc(BUCKETS);
        // data1->ownStorage();
        // *data1->storage = vector<d>(MAX_PARTICLES);
        // data1->bufferData();
        // data2->tryRealloc(MAX_PARTICLES);
        atomics->ownStorage();
        atomics->storage->push_back(0);
        particleShader = _shader("res/shaders/particles.vert", "res/shaders/particles.geom", "res/shaders/particles.frag");
        // renderPrepShader = Shader("res/shaders/renderParticle.comp");
        if (this->VAO == 0)
        {
            glGenVertexArrays(1, &this->VAO);
        }

        // input->tryRealloc(MAX_PARTICLES);
        // _output->tryRealloc(MAX_PARTICLES);
        keys_in->tryRealloc(MAX_PARTICLES);
        keys_out->tryRealloc(MAX_PARTICLES);
        renderParticles->tryRealloc(MAX_PARTICLES);
        input->ownStorage();
        input->storage->resize(MAX_PARTICLES);
        _output->ownStorage();
        _output->storage->resize(MAX_PARTICLES);
        block_sums->ownStorage();
        block_sums->storage->resize(BLOCK_SUM_SIZE);
        scan->ownStorage();
        scan->storage->resize(BUCK * N_GROUPS);
        histo->ownStorage();
        // histo->storage->resize(BUCK * N_GROUPS);
        histo->storage->resize(65536);

        input->bufferData();
        _output->bufferData();
        block_sums->bufferData();
        scan->bufferData();
        histo->bufferData();
        // renderParticles->tryRealloc(MAX_PARTICLES);
    }

    void end(){
        output << (sort1 ? "sort1: " : "sort2: ") << time.getAverageValue() << endl;
        output.close();
    }
    bool flip = true;
    void sortParticles(mat4 vp, mat4 view, vec3 camPos, vec2 screen)
    {
        gpuTimer t1;
        GLuint program;
        particleSortProgram.Use();
        program = particleSortProgram.Program;

        GLuint _vp = glGetUniformLocation(program, "vp");
        GLuint _view = glGetUniformLocation(program, "view");
        GLuint stage = glGetUniformLocation(program, "stage");
        GLuint count = glGetUniformLocation(program, "count");
        GLuint max_particles = glGetUniformLocation(program, "max_particles");
        GLuint num_elements = glGetUniformLocation(program, "numElements");
        GLuint breadth = glGetUniformLocation(program, "breadth");
        glUniformMatrix4fv(_vp, 1, GL_FALSE, glm::value_ptr(vp));
        GLuint nkeys = glGetUniformLocation(program, "nkeys");
        GLuint _pass = glGetUniformLocation(program, "pass");
        GLuint wg_size = glGetUniformLocation(program, "wg_size");
        GLuint _offset = glGetUniformLocation(program, "offset");
        glUniformMatrix4fv(_view, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix3fv(glGetUniformLocation(program, "camInv"), 1, GL_FALSE, glm::value_ptr(camInv));
        glUniform3f(glGetUniformLocation(program, "camPos"), camPos.x, camPos.y, camPos.z);
        glUniform3f(glGetUniformLocation(program, "camp"), camP.x, camP.y, camP.z);
        glUniform3f(glGetUniformLocation(program, "cameraForward"), MainCamForward.x, MainCamForward.y, MainCamForward.z);
        glUniform3f(glGetUniformLocation(program, "cameraUp"), mainCamUp.x, mainCamUp.y, mainCamUp.z);


        glUniform1f(glGetUniformLocation(program, "x_size"),screen.x);
        glUniform1f(glGetUniformLocation(program, "y_size"),screen.y);
        
        // glFlush();
        // data1->retrieveData();
        gpuTimer gt2;
        t1.start();
        atomics->storage->at(0) = 0;
        atomics->bufferData();

        GPU_TRANSFORMS->bindData(0);
        renderParticles->bindData(13);
        input->bindData(1);
        _output->bindData(2);
        keys_in->bindData(11);
        keys_out->bindData(12);
        block_sums->bindData(3);
        particles->bindData(4);
        atomics->bindData(5);
        scan->bindData(6);
        histo->bindData(7);
        // renderParticles->bindData(8);
        gpu_emitter_prototypes->bindData(9);
        gpu_emitters->bindData(10);

        gt2.start();
        glUniform1i(stage, -2);
        glUniform1ui(count, 65536);
        glDispatchCompute(65536 / 128, 1, 1); // count
        glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);

        glUniform1i(stage, -1);
        glUniform1ui(count, MAX_PARTICLES);
        glDispatchCompute(MAX_PARTICLES / 128, 1, 1);
        glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);

        uint numParticles;
        atomics->retrieveData();
        numParticles = atomics->storage->at(0);

        // glUniform1i(stage, -3);
        // glUniform1ui(count, numParticles);
        // glDispatchCompute(numParticles / 128 + 1, 1, 1);
        // glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);


        appendStat("sort particle list stage -2,-1", gt2.stop());
        

        flip = true;
        // glUniform1ui(nkeys, numParticles);
        // glUniform1ui(_offset, 0);
        int start = 0;
        input->bindData(1);   // input
        _output->bindData(2); // output
        flip = !flip;

        // if (sort1)
        // {
            gt2.start();
            glUniform1i(stage, 0);
            glUniform1ui(count, (ceil(numParticles / 32) / 128 + 1) * 128);
            glUniform1ui(nkeys, numParticles);
            glDispatchCompute(ceil(numParticles / 32) / 128 + 1, 1, 1); // count
            glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
            appendStat("sort particle list stage 0", gt2.stop());
        // }


        gt2.start();
        glUniform1i(stage, 1);
        glUniform1ui(count, 256);
        glDispatchCompute(256 / 128, 1, 1); // count
        glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);

        glUniform1i(stage, 2);
        glUniform1ui(count, 1);
        glDispatchCompute(1, 1, 1); // count
        glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
        if(!sort1){
            atomics->retrieveData();
        }
        glUniform1i(stage, 3);
        glUniform1ui(count, 65536);
        glDispatchCompute(65536 / 128, 1, 1); // count
        glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
        appendStat("sort particle list stage 1,2,3", gt2.stop());

        gt2.start();
        glUniform1i(stage, 4);
        glUniform1ui(count, numParticles);
        glDispatchCompute(numParticles / 128 + 1, 1, 1); // count
        glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);

        numParticles = atomics->storage->at(0);

        appendStat("sort particle list stage 4", gt2.stop());

        double t = t1.stop();
        appendStat("sort particle list", t);
        time.add(t);
        // _output->retrieveData();
        // t1.start();

        // renderPrepShader.Use();
        // glUniform1i(stage, 0);
        // glUniform1ui(count, numParticles);
        // glDispatchCompute(numParticles / 128 + 1, 1, 1); // count
        // glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
        // appendStat("make render particles",t1.stop());
    }

    void prepRender()
    {
        GPU_TRANSFORMS->bindData(0);
        input->bindData(1);
        _output->bindData(2);
        block_sums->bindData(3);
        particles->bindData(4);
        atomics->bindData(5);
        scan->bindData(6);
        histo->bindData(7);
        // renderParticles->bindData(8);
        gpu_emitter_prototypes->bindData(9);
        gpu_emitters->bindData(10);
    }
    void drawParticles(mat4 view, mat4 rot, mat4 proj)
    {

        glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
        float farplane = 1e32f;
        particleShader.s->shader->Use();
        GLuint matPView = glGetUniformLocation(particleShader.s->shader->Program, "view");
        GLuint matvRot = glGetUniformLocation(particleShader.s->shader->Program, "vRot");
        GLuint matProjection = glGetUniformLocation(particleShader.s->shader->Program, "projection");
        GLuint cam = glGetUniformLocation(particleShader.s->shader->Program, "cameraPos");
        glUniform3f(cam, mainCamPos.x, mainCamPos.y, mainCamPos.z);

        glUniform1f(glGetUniformLocation(particleShader.s->shader->Program, "aspectRatio"), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT);
        glUniform1f(glGetUniformLocation(particleShader.s->shader->Program, "FC"), 2.0 / log2(farplane + 1));
        glUniform1f(glGetUniformLocation(particleShader.s->shader->Program, "screenHeight"), (float)SCREEN_HEIGHT);
        glUniform1f(glGetUniformLocation(particleShader.s->shader->Program, "screenWidth"), (float)SCREEN_WIDTH);

        glUniformMatrix4fv(matPView, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(matvRot, 1, GL_FALSE, glm::value_ptr(rot));
        glUniformMatrix4fv(matProjection, 1, GL_FALSE, glm::value_ptr(proj));

        GPU_TRANSFORMS->bindData(0);
        gpu_emitter_prototypes->bindData(3);
        gpu_emitters->bindData(4);
        // renderParticles->bindData(8);
        particles->bindData(2);

        // data1->bindData(6);
        // if (!flip)
        _output->bindData(6);
        // else
        //     input->bindData(6);

        glBindVertexArray(this->VAO);

        glDrawArrays(GL_POINTS, 0, atomics->storage->at(0));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // glFlush();
    }
} particle_renderer;
