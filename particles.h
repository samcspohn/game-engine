#include "Component.h"
#include "fast_list.h"
#include <map>
#include "rendering.h"

using namespace std;
using namespace glm;

#define MAX_PARTICLES 1024 * 1024 * 4
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
    float p;
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

    vec3 p;
    int frame;
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
gpu_vector<emitter> *gpu_emitters = new gpu_vector<emitter>();
gpu_vector_proxy<GLint> *gpu_emitter_last_particle = new gpu_vector_proxy<GLint>();

class particle_emitter : public component
{
public:
    emitter_prototype_ prototype;
    typename array_heap<emitter>::ref emitter;
    typename array_heap<GLint>::ref emitter_last_particle;
    COPY(particle_emitter);
    void lateUpdate()
    {
        if (emitter->frame++ > 0)
            emitter->emission -= (float)(int)emitter->emission;
        emitter->emission += prototype->emission_rate * Time.deltaTime;
        emitter->emitter_prototype = prototype.getId();
        // num_particles += (int)emitter->emission;
        // emitter->transform = transform->_T.index;
    }
    void onStart()
    {
        this->emitter = emitters._new();
        this->emitter_last_particle = emitter_last_particles._new();
        this->emitter->transform = transform->_T.index;
        this->emitter->live = 1;
        this->emitter->emission = 1;
        emitter->frame = 0;
        // emitter->last_particle = -1;
    }
    void onDestroy()
    {
        this->emitter->live = 0;
        this->emitter->emission = 0;
        emitters._delete(this->emitter);
        emitter_last_particles._delete(this->emitter_last_particle);
        // emitter->last_particle = -1;
    }
    LATE_UPDATE(particle_emitter, lateUpdate);
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

    gpu_emitter_last_particle->tryRealloc(1024 * 1024);
    particlesToDestroy->tryRealloc(MAX_PARTICLES);
    gpu_emitter_prototypes->storage = &emitter_prototypes_.data;
    gpu_emitters->storage = &emitters.data;
}

Shader particleSortProgram("res/shaders/particle_sort.comp");
Shader particleProgram("res/shaders/particleUpdate.comp");
void updateParticles()
{

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
    gpu_emitter_last_particle->bindData(7);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, rng->bufferId);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, dead->bufferId);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, atomicCounters->bufferId);

    glUniform1fv(glGetUniformLocation(particleProgram.Program, "deltaTime"), 1, &Time.deltaTime);
    float t = Time.time;
    uint32 max_particles = MAX_PARTICLES;

    GLuint cam = glGetUniformLocation(particleProgram.Program, "cameraPosition");
    glUniform3f(cam, mainCamPos.x, mainCamPos.y, mainCamPos.z);
    glUniform3f(glGetUniformLocation(particleProgram.Program, "cameraUp"), mainCamUp.x, mainCamUp.y, mainCamUp.z);
    glUniform3f(glGetUniformLocation(particleProgram.Program, "cameraForward"), MainCamForward.x, MainCamForward.y, MainCamForward.z);

    glUniform1fv(glGetUniformLocation(particleProgram.Program, "time"), 1, &t);
    glUniform1ui(glGetUniformLocation(particleProgram.Program, "max_particles"), max_particles);
    glUniform1ui(glGetUniformLocation(particleProgram.Program, "stage"), 0);
    glUniform1ui(glGetUniformLocation(particleProgram.Program, "count"), emitters.size());
    glDispatchCompute(emitters.size() / 192 + 1, 1, 1);
    glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
    // glFlush();

    glUniform1ui(glGetUniformLocation(particleProgram.Program, "stage"), 1);
    glDispatchCompute(MAX_PARTICLES / 192 + 1, 1, 1);
    glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
    // glFlush();

    atomicCounters->retrieveData();
    // glBindBuffer(GL_SHADER_STORAGE_BUFFER,atomicCounters->bufferId);
    // glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint) * atomicCounters->size(), atomicCounters->storage->data());

    //get data back -- debug info
    // gpu_emitters->retrieveData();
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

struct d
{
    float key;
    int id;
};
#define COUNTER_THREADS 4096
#define BUCKETS 65536
#define buffer_depth 1
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

gpu_vector_proxy<d> *input = new gpu_vector_proxy<d>();
gpu_vector_proxy<d> *_output = new gpu_vector_proxy<d>();
gpu_vector_proxy<GLuint> *block_sums = new gpu_vector_proxy<GLuint>();
gpu_vector_proxy<GLuint> *scan = new gpu_vector_proxy<GLuint>();
gpu_vector_proxy<GLuint> *histo = new gpu_vector_proxy<GLuint>();

class
{
    GLuint VAO = 0;

public:
    _shader particleShader;
    vector<uint> zeros;

    vector<d> res;
    void init()
    {

        if (zeros.size() == 0)
        {
            zeros = vector<uint>(BUCKETS);
        }

        counts->tryRealloc(BUCKETS);
        offsets->tryRealloc(BUCKETS);
        data1->ownStorage();
        *data1->storage = vector<d>(MAX_PARTICLES);
        data1->bufferData();
        data2->tryRealloc(MAX_PARTICLES);
        atomics->ownStorage();
        atomics->storage->push_back(0);
        particleShader = _shader("res/shaders/particles.vert", "res/shaders/particles.geom", "res/shaders/particles.frag");
        if (this->VAO == 0)
        {
            glGenVertexArrays(1, &this->VAO);
        }

        input->tryRealloc(MAX_PARTICLES);
        _output->tryRealloc(MAX_PARTICLES);
        block_sums->tryRealloc(MAX_PARTICLES);
        scan->tryRealloc(MAX_PARTICLES);
        histo->tryRealloc(MAX_PARTICLES);
    }

    void sortParticles(mat4 vp)
    {
        particleSortProgram.Use();
        counts->bindData(0);
        offsets->bindData(1);
        data1->bindData(2);
        data2->bindData(6);
        particles->bindData(4);
        atomics->bindData(5);

        // gpu_init->bindData(7);
        // gpu_init2->bindData(8);
        // localCount->bindData(9);

        counts->bufferData(zeros);
        atomics->storage->at(0) = 0;
        atomics->bufferData();

        GLuint _vp = glGetUniformLocation(particleSortProgram.Program, "vp");
        GLuint stage = glGetUniformLocation(particleSortProgram.Program, "stage");
        GLuint count = glGetUniformLocation(particleSortProgram.Program, "count");
        GLuint max_particles = glGetUniformLocation(particleSortProgram.Program, "max_particles");
        GLuint num_elements = glGetUniformLocation(particleSortProgram.Program, "numElements");
        GLuint breadth = glGetUniformLocation(particleSortProgram.Program, "breadth");
        glUniformMatrix4fv(_vp, 1, GL_FALSE, glm::value_ptr(vp));

        // glUniform1i(stage, -1);
        // glUniform1ui(max_particles, MAX_PARTICLES);
        // glUniform1ui(count, 64 * COUNTER_THREADS);
        // glDispatchCompute(COUNTER_THREADS, 1, 1);
        // glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);

        // glUniform1i(stage, -2);
        // glUniform1ui(count, 64 * COUNTER_THREADS);
        // glDispatchCompute(COUNTER_THREADS, 1, 1);
        // glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);

        glUniform1i(stage, -1);
        glUniform1ui(count, MAX_PARTICLES);
        glDispatchCompute(MAX_PARTICLES / 64, 1, 1);
        glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);

        // glFlush();
        // data1->retrieveData();
        atomics->retrieveData();
        uint numParticles = atomics->storage->at(0);

        // // uint numPasses = log2(double(atomics->storage->at(0))) + 1;
        // uint kernelBreadth = 2;
        // uint numThreads = pow(2,ceil(log2((double)numParticles))) / 2;
        // glUniform1ui(num_elements, numParticles);
        // glUniform1i(stage, 0);
        // for(uint i = 0; numThreads > 0; i++){
        //     glUniform1ui(count, numThreads);
        //     glUniform1ui(breadth, kernelBreadth);
        //     glDispatchCompute(numThreads / 64 + 1, 1, 1);
        //     glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
        //     // glFlush();
        //     kernelBreadth *= 2;
        //     numThreads /= 2;
        // }

        {

            // vector<d>& inter = *data1->storage;

            // size_t end = atomics->storage->at(0);
            // for (auto &i : counters_)
            //     i = 0;
            // for (auto &i : offsets_)
            //     i = 0;
            // for (size_t i = 0; i < end; i++)
            // {
            //     unsigned int c = *(int *)&inter[i] & 0x0000ffff;
            //     counters_[c]++;
            // }
            // for (size_t i = 1; i < counters_.size(); i++)
            // {
            //     offsets_[i] = offsets_[i - 1] + counters_[i - 1];
            // }
            // for (size_t i = 0; i < end; i++)
            // {
            //     unsigned int c = *(int *)&inter[i] & 0x0000ffff;
            //     res[offsets_[c]++] = inter[i];
            // }

            // for (auto &i : counters_)
            //     i = 0;
            // for (auto &i : offsets_)
            //     i = 0;
            // for (size_t i = 0; i < end; i++)
            // {
            //     unsigned int c = *(int *)&res[i] >> 16;
            //     counters_[c]++;
            // }
            // for (size_t i = 1; i < counters_.size(); i++)
            // {
            //     offsets_[i] = offsets_[i - 1] + counters_[i - 1];
            // }
            // for (size_t i = 0; i < end; i++)
            // {
            //     unsigned int c = *(int *)&res[i] >> 16;
            //     inter[end -  offsets_[c]++] = res[i];
            // }
            // data1->bufferData();
            // glFlush();
        }

        // for (int i = 0; i < 6; i++)
        // {
        //     glUniform1i(stage, i);
        //     if (i == 1 || i == 4)
        //     {
        //         glUniform1ui(count, 1);
        //         glDispatchCompute(1, 1, 1);
        //     }
        //     else
        //     {
        //         if (i == 3)
        //         {
        //             counts->bufferData(zeros);
        //         }
        //         glUniform1ui(count, atomics->storage->at(0));
        //         glDispatchCompute(atomics->storage->at(0) / 64 + 1, 1, 1);
        //     }
        //     glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
        //     // glFlush();
        // }

        GLuint nkeys = glGetUniformLocation(particleSortProgram.Program, "nkeys");
        GLuint pass = glGetUniformLocation(particleSortProgram.Program, "pass");

        block_sums->bindData(9);
        scan->bindData(10);
        histo->bindData(11);

#define WG_SIZE 128
#define N_GROUPS 16
#define BUCK (1 << RADIX)
#define RADIX 4
#define BITS 32
        for (int pass = 0; pass < BITS / RADIX; pass++)
        {
            if (pass % 2 == 0)
            {
                input->bindData(7);
                _output->bindData(8);
            }else{
                input->bindData(8);
                _output->bindData(7);
            }
            glUniform1i(stage, 0);
            glUniform1ui(nkeys, numParticles);
            glDispatchCompute(MAX_PARTICLES / 64, 1, 1);
            glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
        }
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

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, particles->bufferId);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, gpu_emitter_prototypes->bufferId);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, gpu_emitters->bufferId);
        gpu_emitter_last_particle->bindData(5);
        data1->bindData(6);

        glBindVertexArray(this->VAO);

        glDrawArrays(GL_POINTS, 0, atomics->storage->at(0));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // glFlush();
    }
} particle_renderer;
