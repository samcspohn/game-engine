#include "Component.h"
#include "fast_list.h"
#include <map>
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
    float p2;
    vec4 p3;
};
struct emitter_prototype
{
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
enum particleCounters
{
    liveParticles = 0,
    destroyCounter = 1
};
gpu_vector<GLuint> *rng = new gpu_vector<GLuint>();
gpu_vector<particle> *particles = new gpu_vector<particle>();
// gpu_vector<GLuint> live;
gpu_vector_proxy<GLuint> *dead = new gpu_vector_proxy<GLuint>();
gpu_vector_proxy<GLuint> *particlesToDestroy = new gpu_vector_proxy<GLuint>();
gpu_vector<GLuint> *atomicCounters = new gpu_vector<GLuint>();

array_heap<emitter_prototype> emitter_prototypes_;
gpu_vector<emitter_prototype> *gpu_emitter_prototypes = new gpu_vector<emitter_prototype>();
map<string, typename array_heap<emitter_prototype>::ref> emitter_prototypes;

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
array_heap<GLint> emitter_last_particles;
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
        emitter_inits[ei.id] = ei;
    }
    // void lateUpdate()
    // {
    //     if (emitter->frame++ > 0)
    //         emitter->emission -= (float)(int)emitter->emission;
    //     emitter->emission += prototype->emission_rate * Time.deltaTime;
    //     emitter->emitter_prototype = prototype.getId();
    //     // num_particles += (int)emitter->emission;
    //     emitter->transform = transform->_T.index;
    // }
    void onStart()
    {
        this->emitter = emitters._new();
        this->emitter_last_particle = emitter_last_particles._new();
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
        emitter_inits[ei.id] = ei;

        // emitter->last_particle = -1;
    }
    void onDestroy()
    {
        this->emitter->live = 0;
        this->emitter->emission = 0;
        emitters._delete(this->emitter);
        emitter_last_particles._delete(this->emitter_last_particle);
        updateEmitters = true;

        emitterInit ei;
        ei.emitterProtoID = prototype.getId();
        ei.live = 0;
        ei.transformID = transform->_T.index;
        ei.id = this->emitter.index;
        emitter_inits[ei.id] = ei;

        // emitter->last_particle = -1;
    }
    // LATE_UPDATE(particle_emitter, lateUpdate);
};

void initParticles()
{

    vector<GLuint> indexes(MAX_PARTICLES);
    for (int i = 0; i < MAX_PARTICLES; ++i)
    {
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
    // gpu_emitters->storage = &emitters.data;
    gpu_emitters->tryRealloc(1024 * 1024 * 4);
    // gpu_emitter_inits->storage = &emitter_inits_db;
}

Shader particleSortProgram("res/shaders/particle_sort.comp");
// Shader particleSortProgram2("res/shaders/particle_sort2.comp");
Shader particleSortProgram2("res/shaders/particleUpdate.comp");
Shader particleProgram("res/shaders/particleUpdate.comp");
void updateParticles(vec3 floatingOrigin, uint emitterInitCount)
{

    gpu_emitter_prototypes->bufferData();
    if (updateEmitters)
    {
        // gpu_emitters->bufferData();
        updateEmitters = false;
    }

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    // run shader update

    glUseProgram(particleProgram.Program);

    // glUniformMatrix4fv(matPView, 1, GL_FALSE, glm::value_ptr(rj.view));
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, GPU_TRANSFORMS->bufferId);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, particles->bufferId);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, gpu_emitter_prototypes->bufferId);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, gpu_emitters->bufferId);
    gpu_emitter_inits->bindData(8);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, rng->bufferId);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, dead->bufferId);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, atomicCounters->bufferId);

    float t = Time.time;
    uint32 max_particles = MAX_PARTICLES;
    glUniform1fv(glGetUniformLocation(particleProgram.Program, "time"), 1, &t);
    glUniform1fv(glGetUniformLocation(particleProgram.Program, "deltaTime"), 1, &Time.deltaTime);
    glUniform3f(glGetUniformLocation(particleProgram.Program, "cameraPosition"), mainCamPos.x, mainCamPos.y, mainCamPos.z);
    glUniform3f(glGetUniformLocation(particleProgram.Program, "cameraUp"), mainCamUp.x, mainCamUp.y, mainCamUp.z);
    glUniform3f(glGetUniformLocation(particleProgram.Program, "cameraForward"), MainCamForward.x, MainCamForward.y, MainCamForward.z);
    glUniform1ui(glGetUniformLocation(particleProgram.Program, "max_particles"), max_particles);
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

    glUniform1ui(glGetUniformLocation(particleProgram.Program, "stage"), 1);
    glDispatchCompute(MAX_PARTICLES / 128 + 1, 1, 1);
    glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);

    atomicCounters->retrieveData();
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
    void sortParticles(mat4 vp, mat4 view, vec3 camPos)
    {
        gpuTimer t1;
        GLuint program;

        if (sort1)
        {
            particleSortProgram.Use();
            program = particleSortProgram.Program;
        }
        else
        {
            particleSortProgram2.Use();
            program = particleSortProgram2.Program;
        }

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
        glUniform3f(glGetUniformLocation(program, "camPos"), camPos.x, camPos.y, camPos.z);
        // glFlush();
        // data1->retrieveData();
        gpuTimer gt2;
        t1.start();
        atomics->storage->at(0) = 0;
        atomics->bufferData();

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

        gt2.start();
        glUniform1i(stage, -2);
        glUniform1ui(count, 65536);
        glDispatchCompute(65536 / 128, 1, 1); // count
        glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);

        glUniform1i(stage, -1);
        glUniform1ui(count, MAX_PARTICLES);
        glDispatchCompute(MAX_PARTICLES / 128, 1, 1);
        glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
        appendStat("sort particle list stage -2,-1", gt2.stop());
        
        uint numParticles;
        if (sort1)
        {
            atomics->retrieveData();
            numParticles = atomics->storage->at(0);
        }
        else
        {
            numParticles = MAX_PARTICLES;
        }

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
