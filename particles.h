#include "Component.h"
#include "fast_list.h"
#include <map>
#include "rendering.h"

using namespace std;
using namespace glm;

#define MAX_PARTICLES 1024 * 1024 * 10
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

#define WG_SIZE 128
#define N_GROUPS 16
#define BUCK (1 << 8)
#define RADIX 16
#define BITS 32

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

class
{
    GLuint VAO = 0;

public:
    _shader particleShader;
    vector<uint> zeros;

    vector<d> res;
    void init()
    {
        cout << endl
             << "1 << 8: " << (1 << 8) << endl;
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
        block_sums->storage->resize(64);
        scan->ownStorage();
        scan->storage->resize(BUCK * N_GROUPS);
        histo->ownStorage();
        histo->storage->resize(BUCK * N_GROUPS);

        input->bufferData();
        _output->bufferData();
        block_sums->bufferData();
        scan->bufferData();
        histo->bufferData();
    }
    bool flip = true;

    void sortParticles(mat4 vp)
    {
        particleSortProgram.Use();
        // counts->bindData(0);
        // offsets->bindData(1);
        // data1->bindData(2);
        // data2->bindData(6);
        particles->bindData(4);
        atomics->bindData(5);

        input->bindData(1);

        // gpu_init->bindData(7);
        // gpu_init2->bindData(8);
        // localCount->bindData(9);

        // counts->bufferData(zeros);
        atomics->storage->at(0) = 0;
        atomics->bufferData();

        GLuint _vp = glGetUniformLocation(particleSortProgram.Program, "vp");
        GLuint stage = glGetUniformLocation(particleSortProgram.Program, "stage");
        GLuint count = glGetUniformLocation(particleSortProgram.Program, "count");
        GLuint max_particles = glGetUniformLocation(particleSortProgram.Program, "max_particles");
        GLuint num_elements = glGetUniformLocation(particleSortProgram.Program, "numElements");
        GLuint breadth = glGetUniformLocation(particleSortProgram.Program, "breadth");
        glUniformMatrix4fv(_vp, 1, GL_FALSE, glm::value_ptr(vp));
        GLuint nkeys = glGetUniformLocation(particleSortProgram.Program, "nkeys");
        GLuint _pass = glGetUniformLocation(particleSortProgram.Program, "pass");
        GLuint wg_size = glGetUniformLocation(particleSortProgram.Program, "wg_size");
        GLuint _offset = glGetUniformLocation(particleSortProgram.Program, "offset");
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
        glUniform1ui(wg_size, 128);

        glUniform1ui(count, MAX_PARTICLES);
        glDispatchCompute(MAX_PARTICLES / 128, 1, 1);
        glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);

        // glFlush();
        // data1->retrieveData();
        atomics->retrieveData();
        uint numParticles = atomics->storage->at(0);
        // numParticles = MAX_PARTICLES;

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
        } {

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
        }

        block_sums->bindData(3);
        scan->bindData(6);
        histo->bindData(7);

        flip = true;
        glUniform1ui(nkeys, numParticles);
        glUniform1ui(_offset, 0);
        int start = 0;
        for (int pass = start, end = 4; pass < end; pass++)
        {
            if (flip)
            {
                input->bindData(1);   // input
                _output->bindData(2); // output
            }
            else
            {
                _output->bindData(1); // input
                input->bindData(2);   // output
            }
            flip = !flip;
            glUniform1ui(_pass, pass);

            glUniform1i(stage, 0);
            glUniform1ui(count, 16 * 128);
            glUniform1ui(wg_size, 128);
            glDispatchCompute(16, 1, 1); // count
            glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
            glFlush();
            // glUniform1i(stage, 1);
            // glUniform1ui(count, 1);
            // glUniform1ui(wg_size, 1);
            // glDispatchCompute(1, 1, 1);
            // glMemoryBarrier(GL_UNIFORM_BARRIER_BIT); // scan

            glUniform1i(stage, 1);
            glUniform1ui(count, 64);
            glDispatchCompute(64 / 128 + 1, 1, 1);
            glMemoryBarrier(GL_UNIFORM_BARRIER_BIT); // scan
            glFlush();
            glUniform1i(stage, 2);
            glUniform1ui(count, 1);
            glDispatchCompute(1, 1, 1);
            glMemoryBarrier(GL_UNIFORM_BARRIER_BIT); // block sum
            glFlush();
            glUniform1i(stage, 3);
            glUniform1ui(count, 16 * BUCK);
            glDispatchCompute(16 * BUCK / 128, 1, 1);
            glMemoryBarrier(GL_UNIFORM_BARRIER_BIT); // coalesce
            glFlush();
            glUniform1i(stage, 4);
            glUniform1ui(count, 16 * 128);
            glUniform1ui(wg_size, 128);
            glDispatchCompute(16, 1, 1);
            glMemoryBarrier(GL_UNIFORM_BARRIER_BIT); // Reorder
            glFlush();
            histo->retrieveData();
        }

        //         int starts[] = {6,4,0};
        //         int ends[] = {8,8,8};
        //         int offset = 0;
        //         int divisors[] = {0,4,16};
        //         for (int grain = 0; grain <= 2; grain++)
        //         {
        //             glUniform1ui(nkeys, numParticles / pow(2, divisors[grain]));
        //             glUniform1ui(_offset, numParticles - numParticles / pow(2, divisors[grain]));
        //             int start = starts[grain];
        //             for (int pass = start, end = ends[grain]; pass < end; pass++)
        //             {
        //                 if (flip)
        //                 {
        //                     input->bindData(1);   // input
        //                     _output->bindData(2); // output
        //                 }
        //                 else
        //                 {
        //                     _output->bindData(1); // input
        //                     input->bindData(2);   // output
        //                 }
        //                 flip = !flip;

        // glFlush();
        //                 glUniform1ui(_pass, pass);

        //                 glUniform1i(stage, 0);
        //                 glUniform1ui(count, 16 * 128);
        //                 glUniform1ui(wg_size, 128);
        //                 glDispatchCompute(16, 1, 1); // count
        //                 glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
        // glFlush();
        //                 // glUniform1i(stage, 1);
        //                 // glUniform1ui(count, 1);
        //                 // glUniform1ui(wg_size, 1);
        //                 // glDispatchCompute(1, 1, 1);
        //                 // glMemoryBarrier(GL_UNIFORM_BARRIER_BIT); // scan

        //                 glUniform1i(stage, 1);
        //                 glUniform1ui(count, 256);
        //                 glDispatchCompute(256 / 128, 1, 1);
        //                 glMemoryBarrier(GL_UNIFORM_BARRIER_BIT); // scan
        // glFlush();
        //                 glUniform1i(stage, 2);
        //                 glUniform1ui(count, 1);
        //                 glDispatchCompute(1, 1, 1);
        //                 glMemoryBarrier(GL_UNIFORM_BARRIER_BIT); // block sum
        // glFlush();
        //                 glUniform1i(stage, 3);
        //                 glUniform1ui(count, 32768);
        //                 glDispatchCompute(32768 / 128, 1, 1);
        //                 glMemoryBarrier(GL_UNIFORM_BARRIER_BIT); // coalesce
        // glFlush();
        //                 glUniform1i(stage, 4);
        //                 glUniform1ui(count, 16 * 128);
        //                 glUniform1ui(wg_size, 128);
        //                 glDispatchCompute(16, 1, 1);
        //                 glMemoryBarrier(GL_UNIFORM_BARRIER_BIT); // Reorder
        // glFlush();
        //             // block_sums->retrieveData();
        //             // // offset = numParticles - (*block_sums)[32];
        //             // // offset = 0;
        //             // numParticles -= (*block_sums)[32];
        //             }
        //         }
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
        // data1->bindData(6);
        input->bindData(6);
        // _output->bindData(6);

        glBindVertexArray(this->VAO);

        glDrawArrays(GL_POINTS, 0, atomics->storage->at(0));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // glFlush();
    }
} particle_renderer;
