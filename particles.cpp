#include "particles.h"
#include "fast_list.h"
#include <map>
#include <unordered_map>
#include <mutex>
#include <deque>
#include "rendering.h"
#include <fstream>
#include "helper1.h"
#include "gpu_sort.h"
#include "editor.h"
// #include "Shader.h"

using namespace std;
using namespace glm;
#define MAX_PARTICLES 1024 * 1024 * 8

struct smquat
{
    uvec2 d;
};
struct smvec3
{
    uint xy;
    float z;
};
struct particle
{
    vec3 position;
    // uint emitter;

    vec2 scale;
    uint emitter_prototype;
    float life;

    smquat rotation; // 2ints
    smvec3 velocity; // 2ints

    vec3 position2;
    int next;

    smvec3 velocity2; // 2ints
    float p1;
    float l;
};

// vec4 color;
void emitter_prototype::color(vec4 c)
{
    for (int i = 0; i < 100; ++i)
    {
        colorLife[i] = c;
    }
}
void emitter_prototype::color(vec4 c1, vec4 c2)
{
    vec4 step = (c2 - c1) / 100.f;
    for (int i = 0; i < 100; ++i)
    {
        colorLife[i] = c1 + step * (float)i;
    }
}
void emitter_prototype::size(float c)
{
    for (int i = 0; i < 100; ++i)
    {
        sizeLife[i] = c;
    }
}
void emitter_prototype::size(float c1, float c2)
{
    float step = (c2 - c1) / 100.f;
    for (int i = 0; i < 100; ++i)
    {
        sizeLife[i] = c1 + step * (float)i;
    }
}

colorArray &colorArray::addKey(vec4 color, float position)
{
    key k;
    k.color = color;
    k.pos = position;
    keys.push_back(k);
    return *this;
}
void colorArray::setColorArray(vec4 *colors)
{
    if (keys.size() == 0)
        return;
    key k = keys.front();
    if (keys.size() == 1)
    {
        for (int i = 0; i < 100; ++i)
        {
            colors[i] = k.color;
        }
    }
    else
    {
        int k1 = 0;
        int k2 = 1;
        for (int i = 0; i < keys[k1].pos * 100; ++i)
        {
            colors[i] = keys[k1].color;
        }
        while (k2 < keys.size())
        {
            vec4 step = (keys[k2].color - keys[k1].color) / ((keys[k2].pos - keys[k1].pos) * 100);
            int start = keys[k1].pos * 100;
            int stop = keys[k2].pos * 100;
            int j = 0;
            for (int i = start; i < stop; i++, j++)
            {
                colors[i] = keys[k1].color + step * (float)j;
            }
            k1 = k2++;
        }
        for (int i = keys[k1].pos * 100; i < 100; ++i)
        {
            colors[i] = keys[k1].color;
        }
    }
}

floatArray &floatArray::addKey(float v, float position)
{
    key k;
    k.value = v;
    k.pos = position;
    keys.push_back(k);
    return *this;
}
void floatArray::setFloatArray(float *floats)
{
    if (keys.size() == 0)
        return;
    key k = keys.front();
    if (keys.size() == 1)
    {
        for (int i = 0; i < 100; ++i)
        {
            floats[i] = k.value;
        }
    }
    else
    {
        int k1 = 0;
        int k2 = 1;
        for (int i = 0; i < keys[k1].pos * 100; ++i)
        {
            floats[i] = keys[k1].value;
        }
        while (k2 < keys.size())
        {
            float step = (keys[k2].value - keys[k1].value) / ((keys[k2].pos - keys[k1].pos) * 100);
            int start = keys[k1].pos * 100;
            int stop = keys[k2].pos * 100;
            int j = 0;
            for (int i = start; i < stop; i++, j++)
            {
                floats[i] = keys[k1].value + step * (float)j;
            }
            k1 = k2++;
        }
        for (int i = keys[k1].pos * 100; i < 100; ++i)
        {
            floats[i] = keys[k1].value;
        }
    }
}

struct _emission
{
    vec3 position;
    uint emitter_prototype;
    vec3 direction;
    int emitterID;
    vec3 scale;
    int last;
};
struct _burst
{
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
gpu_vector_proxy<particle> *particles = new gpu_vector_proxy<particle>();
gpu_vector_proxy<_emission> *emitted = new gpu_vector_proxy<_emission>();
gpu_vector_proxy<GLuint> *burstParticles = new gpu_vector_proxy<GLuint>();
gpu_vector<_burst> *gpu_particle_bursts = new gpu_vector<_burst>();
vector<_burst> particle_bursts;
gpu_vector_proxy<GLuint> *dead = new gpu_vector_proxy<GLuint>();
gpu_vector_proxy<GLuint> *particlesToDestroy = new gpu_vector_proxy<GLuint>();
gpu_vector<GLuint> *atomicCounters = new gpu_vector<GLuint>();
gpu_vector_proxy<GLuint> *livingParticles = new gpu_vector_proxy<GLuint>();
gpu_vector_proxy<GLuint> *particleLifes = new gpu_vector_proxy<GLuint>();
array_heap<emitter_prototype> emitter_prototypes_;
gpu_vector<emitter_prototype> *gpu_emitter_prototypes = new gpu_vector<emitter_prototype>();
map<string, typename array_heap<emitter_prototype>::ref> emitter_prototypes;
map<int, string> emitter_proto_names;

void saveEmitters(OARCHIVE &oa)
{
    oa << emitter_prototypes_ << emitter_prototypes;
}
void loadEmitters(IARCHIVE &ia)
{
    ia >> emitter_prototypes_ >> emitter_prototypes;
}

mutex burstLock;
void swapBurstBuffer()
{
    gpu_particle_bursts->storage->swap(particle_bursts);
    particle_bursts.clear();
}

int emitter_prototype_::getId()
{
    return emitterPrototype.index;
}
emitter_prototype *emitter_prototype_::operator->()
{
    return &emitterPrototype.data();
}
emitter_prototype &emitter_prototype_::operator*()
{
    return emitterPrototype.data();
}
void emitter_prototype_::burst(glm::vec3 pos, glm::vec3 dir, uint count)
{
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
void emitter_prototype_::burst(glm::vec3 pos, glm::vec3 dir, glm::vec3 scale, uint count)
{
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

emitter_prototype_ createNamedEmitter(string name)
{
    emitter_prototypes.insert(std::pair<string, typename array_heap<emitter_prototype>::ref>(name, emitter_prototypes_._new()));
    emitter_prototype_ ret;
    ret.emitterPrototype = emitter_prototypes.at(name);
    emitter_proto_names[ret.emitterPrototype.index] = name;
    return ret;
}
emitter_prototype_ getNamedEmitterProto(string name)
{
    emitter_prototype_ ret;
    ret.emitterPrototype = emitter_prototypes.at(name);
    return ret;
}
void emitter_prototype_::onEdit()
{

    char input[1024];
	sprintf(input, name.c_str());
	if (ImGui::InputText("", input, 1024, ImGuiInputTextFlags_None))
		name = {input};
	ImGui::PopID();
	ImGui::PopItemWidth();
	ImGui::Button(name.c_str(), {40, 40});
	if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
	{
		// Set payload to carry the index of our item (could be anything)
		ImGui::SetDragDropPayload("EMITTER_PROTOTYPE_DRAG_AND_DROP", &this->emitterPrototype.index, sizeof(int));
		ImGui::EndDragDropSource();
	}

    // emitterPrototype->edit();

}

void renderEdit(const char *name, emitter_prototype_ &ep){
    ImGui::InputText(name, (char *)emitter_proto_names.at(ep.getId()).c_str(), emitter_proto_names.at(ep.getId()).size() + 1, ImGuiInputTextFlags_ReadOnly);
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("EMITTER_PROTOTYPE_DRAG_AND_DROP"))
		{
			IM_ASSERT(payload->DataSize == sizeof(int));
			int payload_n = *(const int *)payload->Data;
            ep = getNamedEmitterProto(emitter_proto_names.at(payload_n));
		}
		ImGui::EndDragDropTarget();
	}
}

array_heap<emitter> EMITTERS;
gpu_vector_proxy<emitter> *gpu_emitters = new gpu_vector_proxy<emitter>();
gpu_vector_proxy<emitterInit> *gpu_emitter_inits = new gpu_vector_proxy<emitterInit>();
vector<emitterInit> emitterInits;
vector<emitterInit> emitterInitsdb;
unordered_map<uint, emitterInit> emitter_inits;

void renderEdit(string name, emitter_prototype_ &ep)
{
    for (auto &i : emitter_prototypes)
    {
        if (ep.getId() == i.second.index)
            ImGui::Text(i.first.c_str());
    }
}

// COPY(particle_emitter);
void particle_emitter::setPrototype(emitter_prototype_ ep)
{
    prototype = ep;
    emitter->emitter_prototype = ep.getId();

    emitterInit ei;
    ei.emitterProtoID = prototype.getId();
    ei.live = 1;
    ei.transformID = transform.id;
    ei.id = this->emitter.index;
    lock.lock();
    emitter_inits[ei.id] = ei;
    lock.unlock();
}

void particle_emitter::onStart()
{
    this->emitter = EMITTERS._new();
    this->emitter->transform = transform.id;
    this->emitter->live = 1;
    this->emitter->emission = 1;
    emitter->emitter_prototype = prototype.getId();
    emitter->frame = 0;

    emitterInit ei;
    ei.emitterProtoID = prototype.getId();
    ei.live = 1;
    ei.transformID = transform.id;
    ei.id = this->emitter.index;
    lock.lock();
    emitter_inits[ei.id] = ei;
    lock.unlock();
}
void particle_emitter::onDestroy()
{
    this->emitter->live = 0;
    this->emitter->emission = 0;
    EMITTERS._delete(this->emitter);

    emitterInit ei;
    ei.emitterProtoID = prototype.getId();
    ei.live = 0;
    ei.transformID = transform.id;
    ei.id = this->emitter.index;
    lock.lock();
    emitter_inits[ei.id] = ei;
    lock.unlock();
}
void particle_emitter::onEdit()
{
    for (auto &i : emitter_prototypes)
    {
        if (this->prototype.getId() == i.second.index)
            ImGui::Text(i.first.c_str());
    }
}
REGISTER_COMPONENT(particle_emitter)

mutex particle_emitter::lock;
void initParticles()
{
    cout << "size of particle: " << sizeof(particle) << endl;
    cout << "size of particle array: " << sizeof(particle) * MAX_PARTICLES << endl;
    vector<GLuint> indexes(MAX_PARTICLES);
    for (int i = 0; i < MAX_PARTICLES; ++i)
    {
        indexes[i] = i;
    }
    vector<GLuint> _0s = vector<GLuint>(MAX_PARTICLES);
    dead->bufferData(indexes);
    // particles->ownStorage();
    // *particles->storage = vector<particle>(MAX_PARTICLES);
    // particles->bufferData();
    particles->tryRealloc(MAX_PARTICLES);
    atomicCounters->ownStorage();
    *atomicCounters->storage = vector<GLuint>(3);
    atomicCounters->bufferData();

    gpu_particle_bursts->ownStorage();

    livingParticles->tryRealloc(MAX_PARTICLES);
    // emitted->tryRealloc(MAX_PARTICLES);
    burstParticles->tryRealloc(MAX_PARTICLES);
    // burstParticles->bufferData();
    particlesToDestroy->tryRealloc(MAX_PARTICLES);
    particleLifes->tryRealloc(MAX_PARTICLES);
    particleLifes->bufferData(_0s);
    gpu_emitter_prototypes->storage = &emitter_prototypes_.data;

    // gpu_emitters->tryRealloc(1024 * 1024 * 4);
}

Shader particleSortProgram("res/shaders/particle_sort_1.comp");
// Shader particleSortProgram2("res/shaders/particle_sort2.comp");
Shader particleSortProgram2("res/shaders/particle_sort_2.comp");
Shader particleProgram("res/shaders/particleUpdate_.comp");
Shader particleProgram2("res/shaders/particleUpdate_burst.comp");
Shader particleProgram3("res/shaders/particleUpdate_emitter.comp");
int particleCount;
int actualParticles;
mutex pcMutex;
int getParticleCount()
{
    int ret;
    pcMutex.lock();
    ret = particleCount;
    pcMutex.unlock();
    return ret;
}
void updateParticles(vec3 floatingOrigin, uint emitterInitCount)
{

    // particleProgram.use();
    //prepare program. bind variables
    GPU_TRANSFORMS->bindData(0);
    atomicCounters->bindData(1);
    particles->bindData(2);
    gpu_emitter_prototypes->bindData(3);
    gpu_emitters->resize(EMITTERS.size());
    gpu_emitters->bindData(4);
    burstParticles->bindData(5);
    dead->bindData(6);
    // emitted->bindData(7);
    particleLifes->bindData(7);
    gpu_emitter_inits->bindData(8);
    gpu_particle_bursts->bindData(9);
    livingParticles->bindData(10);

    float t = Time.time;
    int32 max_particles = MAX_PARTICLES;
    particleProgram.use();

    glUniform1fv(glGetUniformLocation(particleProgram.Program, "time"), 1, &t);
    glUniform1fv(glGetUniformLocation(particleProgram.Program, "deltaTime"), 1, &Time.deltaTime);
    glUniform3f(glGetUniformLocation(particleProgram.Program, "cameraPosition"), mainCamPos.x, mainCamPos.y, mainCamPos.z);
    glUniform3f(glGetUniformLocation(particleProgram.Program, "cameraUp"), mainCamUp.x, mainCamUp.y, mainCamUp.z);
    glUniform3f(glGetUniformLocation(particleProgram.Program, "cameraForward"), MainCamForward.x, MainCamForward.y, MainCamForward.z);
    glUniform1i(glGetUniformLocation(particleProgram.Program, "max_particles"), MAX_PARTICLES);
    GLuint fo = glGetUniformLocation(particleProgram.Program, "floatingOrigin");
    glUniform3f(fo, floatingOrigin.x, floatingOrigin.y, floatingOrigin.z);

    particleProgram2.use();

    glUniform1fv(glGetUniformLocation(particleProgram2.Program, "time"), 1, &t);
    glUniform1fv(glGetUniformLocation(particleProgram2.Program, "deltaTime"), 1, &Time.deltaTime);
    glUniform3f(glGetUniformLocation(particleProgram2.Program, "cameraPosition"), mainCamPos.x, mainCamPos.y, mainCamPos.z);
    glUniform3f(glGetUniformLocation(particleProgram2.Program, "cameraUp"), mainCamUp.x, mainCamUp.y, mainCamUp.z);
    glUniform3f(glGetUniformLocation(particleProgram2.Program, "cameraForward"), MainCamForward.x, MainCamForward.y, MainCamForward.z);
    glUniform1i(glGetUniformLocation(particleProgram2.Program, "max_particles"), MAX_PARTICLES);
    fo = glGetUniformLocation(particleProgram2.Program, "floatingOrigin");
    glUniform3f(fo, floatingOrigin.x, floatingOrigin.y, floatingOrigin.z);

    particleProgram3.use();

    glUniform1fv(glGetUniformLocation(particleProgram3.Program, "time"), 1, &t);
    glUniform1fv(glGetUniformLocation(particleProgram3.Program, "deltaTime"), 1, &Time.deltaTime);
    glUniform3f(glGetUniformLocation(particleProgram3.Program, "cameraPosition"), mainCamPos.x, mainCamPos.y, mainCamPos.z);
    glUniform3f(glGetUniformLocation(particleProgram3.Program, "cameraUp"), mainCamUp.x, mainCamUp.y, mainCamUp.z);
    glUniform3f(glGetUniformLocation(particleProgram3.Program, "cameraForward"), MainCamForward.x, MainCamForward.y, MainCamForward.z);
    glUniform1i(glGetUniformLocation(particleProgram3.Program, "max_particles"), MAX_PARTICLES);
    fo = glGetUniformLocation(particleProgram3.Program, "floatingOrigin");
    glUniform3f(fo, floatingOrigin.x, floatingOrigin.y, floatingOrigin.z);

    particleProgram3.use();

    // run program
    glUniform1ui(glGetUniformLocation(particleProgram3.Program, "count"), emitterInitCount);
    glUniform1ui(glGetUniformLocation(particleProgram3.Program, "stage"), 0);
    glDispatchCompute(emitterInitCount / 128 + 1, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    glUniform1ui(glGetUniformLocation(particleProgram3.Program, "count"), EMITTERS.size());
    glUniform1ui(glGetUniformLocation(particleProgram3.Program, "stage"), 1);
    glDispatchCompute(EMITTERS.size() / 128 + 1, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    atomicCounters->retrieveData();
    (*atomicCounters)[1] = 0;
    (*atomicCounters)[2] = 0;
    atomicCounters->bufferData();
    // glFlush();

    vector<uint> acs = *(atomicCounters->storage);

    particleProgram2.use();
    glUniform1ui(glGetUniformLocation(particleProgram2.Program, "burstOffset"), (*atomicCounters)[0]);

    glUniform1ui(glGetUniformLocation(particleProgram2.Program, "count"), gpu_particle_bursts->size());
    glUniform1ui(glGetUniformLocation(particleProgram2.Program, "stage"), 2);
    glDispatchCompute(gpu_particle_bursts->size() / 128 + 1, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    atomicCounters->retrieveData(); //TODO // replace with dispatch indirect

    glUniform1ui(glGetUniformLocation(particleProgram2.Program, "count"), (*atomicCounters)[1]);
    glUniform1ui(glGetUniformLocation(particleProgram2.Program, "stage"), 3);
    glDispatchCompute((*atomicCounters)[1] / 128 + 1, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    particleProgram.use();
    glUniform1ui(glGetUniformLocation(particleProgram.Program, "count"), 1); // if particle emissions exceed buffer
    glUniform1ui(glGetUniformLocation(particleProgram.Program, "stage"), 4);
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    glUniform1ui(glGetUniformLocation(particleProgram.Program, "count"), 1); // reset particle counter
    glUniform1ui(glGetUniformLocation(particleProgram.Program, "stage"), 5);
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    glUniform1ui(glGetUniformLocation(particleProgram.Program, "count"), MAX_PARTICLES); // count particles
    glUniform1ui(glGetUniformLocation(particleProgram.Program, "stage"), 6);
    glDispatchCompute(MAX_PARTICLES / 256 + 1, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    atomicCounters->retrieveData();
    pcMutex.lock();
    particleCount = atomicCounters->storage->at(0);
    actualParticles = atomicCounters->storage->at(2);
    pcMutex.unlock();

    glUniform1ui(glGetUniformLocation(particleProgram.Program, "count"), actualParticles);
    glUniform1ui(glGetUniformLocation(particleProgram.Program, "stage"), 7);
    glDispatchCompute(actualParticles / 256 + 1, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

struct d
{
    // smvec3 pos;
    uint xy;
    uint z;
    smquat rot;
    uint scale_xy;
    uint protoID_life;
    uint p1;
    uint p2;
};

#define COUNTER_THREADS 4096
#define BUCKETS 65536
#define buffer_depth 1

#define N_GROUPS 256
#define RADIX 12
#define BUCK (1 << RADIX)
#define BITS 32
#define BLOCK_SUM_SIZE 256
// sqrt(N_GROUPS * BUCK)
gpu_vector_proxy<uint> *keys_in = new gpu_vector_proxy<uint>();
gpu_vector_proxy<uint> *keys_out = new gpu_vector_proxy<uint>();
gpu_vector<uint> *atomics = new gpu_vector<uint>();
vector<int> counters_(BUCKETS);
vector<int> offsets_(BUCKETS);

gpu_vector_proxy<d> *_input = new gpu_vector_proxy<d>();
gpu_vector_proxy<d> *_output = new gpu_vector_proxy<d>();
gpu_vector<GLuint> *block_sums = new gpu_vector<GLuint>();
gpu_vector<GLuint> *histo = new gpu_vector<GLuint>();

namespace particle_renderer
{
    GLuint VAO = 0;
    glm::mat3 camInv;
    glm::vec3 camP;

    _shader particleShader;
    vector<uint> zeros;

    vector<d> res;
    ofstream output;
    rolling_buffer time;
    sorter<d> *p_sort;

    void setCamCull(glm::mat3 ci, glm::vec3 cp)
    {
        camInv = ci;
        camP = cp;
    }
    void init()
    {
        time = rolling_buffer(1000);
        output = ofstream("particle_perf.txt", ios_base::app);
        if (zeros.size() == 0)
        {
            zeros = vector<uint>(BUCKETS);
        }
        atomics->ownStorage();
        atomics->storage->push_back(0);
        particleShader = _shader("res/shaders/particles.vert", "res/shaders/particles.geom", "res/shaders/particles.frag");

        if (VAO == 0)
        {
            glGenVertexArrays(1, &VAO);
        }

        keys_in->tryRealloc(MAX_PARTICLES);
        keys_out->tryRealloc(MAX_PARTICLES);
        _input->tryRealloc(MAX_PARTICLES);
        // _output->ownStorage();
        _output->tryRealloc(MAX_PARTICLES);
        block_sums->ownStorage();
        block_sums->storage->resize(BLOCK_SUM_SIZE);
        histo->ownStorage();
        histo->storage->resize(65536);
        block_sums->bufferData();
        histo->bufferData();

        p_sort = new sorter<d>("d",
                               "struct smquat{\
	uvec2 d;\
};\
\
struct d{\
    uint xy;\
    uint z;\
    smquat rot;\
	uint scale_xy;\
	uint protoID_life;\
};",
                               "z");
    }

    void end()
    {
        output << "sort:" << time.getAverageValue() << endl;
        output.close();
    }

    void sortParticles(mat4 vp, mat4 view, vec3 camPos, vec2 screen)
    {
        timer t1;
        particleSortProgram.use();

        particleSortProgram.setMat3("camInv", camInv);
        particleSortProgram.setVec3("camPos", camPos);
        particleSortProgram.setVec3("camp", camP);
        particleSortProgram.setVec3("cameraForward", MainCamForward);
        particleSortProgram.setVec3("cameraUp", mainCamUp);
        particleSortProgram.setFloat("x_size", screen.x);
        particleSortProgram.setFloat("y_size", screen.y);

        // particleSortProgram2.use();
        // particleSortProgram2.setMat3("camInv",camInv);
        // particleSortProgram2.setVec3("camPos", camPos);
        // particleSortProgram2.setVec3("camp",camP);
        // particleSortProgram2.setVec3("cameraForward",MainCamForward);
        // particleSortProgram2.setVec3("cameraUp",mainCamUp);
        // particleSortProgram2.setFloat("x_size",screen.x);
        // particleSortProgram2.setFloat("y_size",screen.y);

        gpuTimer gt;
        t1.start();
        atomics->storage->at(0) = 0;
        atomics->bufferData();
        histo->bufferData();

        livingParticles->bindData(0);
        _input->bindData(1);
        _output->bindData(2);
        block_sums->bindData(3);
        particles->bindData(4);
        atomics->bindData(5);
        histo->bindData(7);
        gpu_emitter_prototypes->bindData(8);

        gt.start();
        particleSortProgram.use();
        // particleSortProgram.setInt("stage",-2);
        // particleSortProgram.setUint("count", 65536);
        // glDispatchCompute(65536 / 256, 1, 1); // count
        // glMemoryBarrier(GL_ALL_BARRIER_BITS);

        particleSortProgram.setInt("stage", -1);
        particleSortProgram.setUint("count", actualParticles);
        glDispatchCompute(actualParticles / 256 + 1, 1, 1);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
        uint numParticles;
        atomics->retrieveData();
        numParticles = atomics->storage->at(0);
        appendStat("particle list create", gt.stop());
        // glMemoryBarrier(GL_ALL_BARRIER_BITS);

        gt.start();
        p_sort->sort(numParticles, _input, _output);
        appendStat("particle list sort", gt.stop());
        // glMemoryBarrier(GL_ALL_BARRIER_BITS);

        // _input->bindData(1);   // input
        // _output->bindData(2); // output

        // particleSortProgram2.use();

        // // particleSortProgram2.setInt("stage", -1);
        // // particleSortProgram2.setUint("count", numParticles);
        // // glDispatchCompute(numParticles / 256 + 1, 1, 1); // count
        // // glMemoryBarrier(GL_ALL_BARRIER_BITS);

        // gt2.start();
        // particleSortProgram2.setInt("stage",0);
        // uint subSortGroups = ceil(numParticles / 8) / 256 + 1;
        // particleSortProgram2.setUint("count", subSortGroups * 256);
        // particleSortProgram2.setUint("nkeys", numParticles);
        // glDispatchCompute(subSortGroups, 1, 1); // count
        // glMemoryBarrier(GL_ALL_BARRIER_BITS);
        // appendStat("sort particle list stage 0", gt2.stop());

        // gt2.start();
        // particleSortProgram2.setInt("stage",1);
        // particleSortProgram2.setUint("count", 256);
        // glDispatchCompute(256 / 256, 1, 1); // count
        // glMemoryBarrier(GL_ALL_BARRIER_BITS);

        // particleSortProgram2.setInt("stage",2);
        // particleSortProgram2.setUint("count", 1);
        // glDispatchCompute(1, 1, 1); // count
        // glMemoryBarrier(GL_ALL_BARRIER_BITS);

        // particleSortProgram2.setInt("stage",3);
        // particleSortProgram2.setUint("count", 65536);
        // glDispatchCompute(65536 / 256, 1, 1); // count
        // glMemoryBarrier(GL_ALL_BARRIER_BITS);
        // appendStat("sort particle list stage 1,2,3", gt2.stop());

        // gt2.start();
        // particleSortProgram2.setInt("stage",4);
        // particleSortProgram2.setUint("count", numParticles);
        // glDispatchCompute(numParticles / 256 + 1, 1, 1); // count
        // glMemoryBarrier(GL_ALL_BARRIER_BITS);
        // appendStat("sort particle list stage 4", gt2.stop());

        // // atomics->retrieveData();
        // // numParticles = atomics->storage->at(0);

        // double t = t1.stop();
        // appendStat("sort particle list", t);
        // time.add(t);
    }

    void drawParticles(mat4 view, mat4 rot, mat4 proj)
    {

        glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
        float farplane = 1e32f;
        particleShader.meta()->shader->use();
        particleShader->setMat4("view", view);
        // particleShader->setMat4("view",view);
        // GLuint matPView = glGetUniformLocation(particleShader.s->shader->Program, "view");
        particleShader->setMat4("vRot", rot);
        particleShader->setMat4("projection", proj);
        // GLuint matvRot = glGetUniformLocation(particleShader.s->shader->Program, "vRot");
        // GLuint matProjection = glGetUniformLocation(particleShader.s->shader->Program, "projection");
        // GLuint cam = glGetUniformLocation(particleShader.s->shader->Program, "cameraPos");
        // glUniform3f(cam, mainCamPos.x, mainCamPos.y, mainCamPos.z);
        particleShader->setVec3("cameraPos", mainCamPos);
        particleShader->setFloat("aspectRatio", (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT);
        // glUniform1f(glGetUniformLocation(particleShader.s->shader->Program, "aspectRatio"), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT);
        particleShader->setFloat("FC", 2.0 / log2(farplane + 1));
        particleShader->setFloat("screenHeight", (float)SCREEN_HEIGHT);
        particleShader->setFloat("screenWidth", (float)SCREEN_WIDTH);
        // glUniform1f(glGetUniformLocation(particleShader.s->shader->Program, "FC"), 2.0 / log2(farplane + 1));
        // glUniform1f(glGetUniformLocation(particleShader.s->shader->Program, "screenHeight"), (float)SCREEN_HEIGHT);
        // glUniform1f(glGetUniformLocation(particleShader.s->shader->Program, "screenWidth"), (float)SCREEN_WIDTH);
        particleShader->setMat3("camInv", camInv);

        // glUniformMatrix4fv(matPView, 1, GL_FALSE, glm::value_ptr(view));
        // glUniformMatrix4fv(matvRot, 1, GL_FALSE, glm::value_ptr(rot));
        // glUniformMatrix4fv(matProjection, 1, GL_FALSE, glm::value_ptr(proj));

        GPU_TRANSFORMS->bindData(0);
        gpu_emitter_prototypes->bindData(3);
        gpu_emitters->bindData(4);
        particles->bindData(2);
        _input->bindData(6);
        // _output->bindData(6);

        glBindVertexArray(VAO);

        glDrawArrays(GL_POINTS, 0, atomics->storage->at(0));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
}; // namespace particle_renderer

void prepParticles()
{
    gpu_emitter_inits->bufferData(emitterInits);
    gpu_emitter_prototypes->bufferData();
    gpu_particle_bursts->bufferData();
}