#include "particles.h"
#include "fast_list.h"
#include <map>
#include <unordered_map>
#include <mutex>
#include <deque>
#include "_rendering/_renderer.h"
#include <fstream>
#include "helper1.h"
#include "gpu_sort.h"
#include "_serialize.h"
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
    int p1;
    // uint emitter;

    vec2 scale;
    uint emitter_prototype;
    float life;

    smquat rotation; // 2ints
    smvec3 velocity; // 2ints
    // vec3 position2;
    int next;

    float l;
    // smvec3 velocity2; // 2ints
    float p2;
    int p3;
};
struct _emission
{
    vec3 position;
    uint emitter_prototype;
    vec3 direction;
    int emitterID;
    vec3 scale;
    int last;
};

enum particleCounters
{
    liveParticles = 0,
    destroyCounter = 1
};

// simulation
gpu_vector_proxy<particle> *particles = new gpu_vector_proxy<particle>();
gpu_vector_proxy<GLuint> *burstParticles = new gpu_vector_proxy<GLuint>();

gpu_vector_proxy<GLuint> *dead = new gpu_vector_proxy<GLuint>();
gpu_vector<GLuint> *atomicCounters = new gpu_vector<GLuint>();
gpu_vector_proxy<GLuint> *livingParticles = new gpu_vector_proxy<GLuint>();
gpu_vector_proxy<GLuint> *particleLifes = new gpu_vector_proxy<GLuint>();

unique_ptr<Shader> particleSortProgram;
unique_ptr<Shader> particleSortProgram2;
unique_ptr<Shader> particleProgram;
unique_ptr<Shader> particleProgram2;
unique_ptr<Shader> particleProgram3;

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
    particles->tryRealloc(MAX_PARTICLES);
    atomicCounters->ownStorage();
    *atomicCounters->storage = vector<GLuint>(3);
    atomicCounters->bufferData();

    init_prototypes();

    livingParticles->tryRealloc(MAX_PARTICLES);

    burstParticles->tryRealloc(MAX_PARTICLES);
    particleLifes->tryRealloc(MAX_PARTICLES);
    particleLifes->bufferData(_0s);
    gpu_emitter_prototypes->storage = &emitter_prototypes_.data;
}

void initParticles2()
{
    particleSortProgram = make_unique<Shader>("res/shaders/particles/particle_sort_1.comp");
    particleSortProgram2 = make_unique<Shader>("res/shaders/particles/particle_sort_2.comp");
    particleProgram = make_unique<Shader>("res/shaders/particles/particleUpdate_.comp");
    particleProgram2 = make_unique<Shader>("res/shaders/particles/particleUpdate_burst.comp");
    particleProgram3 = make_unique<Shader>("res/shaders/particles/particleUpdate_emitter.comp");

    // emitter_prototype_ default_emitter = createNamedEmitter("default");

    auto ep = make_shared<emitter_proto_asset>();
    ep->id = 0;
    ep->ref = emitter_prototypes_._new();
    emitter_manager.meta[ep->id] = ep;
    // emitter_prototypes.insert(std::pair<string, int>("default", ep->id));
    // // ret.emitterPrototype = emitter_prototypes.at(name);
    // emitter_proto_names[ep->id] = "default";
    ep->name = "default_emitter";
}
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
int getActualParticles()
{
    int ret;
    pcMutex.lock();
    ret = actualParticles;
    pcMutex.unlock();
    return ret;
}
void updateParticles(vec3 floatingOrigin, uint emitterInitCount)
{

    // prepare program. bind variables
    int emitters_size = ComponentRegistry.registry<particle_emitter>()->size();
    GPU_TRANSFORMS->bindData(0);
    atomicCounters->bindData(1);
    particles->bindData(2);
    gpu_emitter_prototypes->bindData(3);
    gpu_emitters->resize(emitters_size);
    gpu_emitters->bindData(4);
    burstParticles->bindData(5);
    dead->bindData(6);
    particleLifes->bindData(7);
    gpu_emitter_inits->bindData(8);
    gpu_particle_bursts->bindData(9);
    livingParticles->bindData(10);

    float t = Time.time;
    particleProgram->use();

    glUniform1fv(glGetUniformLocation(particleProgram->Program, "time"), 1, &t);
    glUniform1fv(glGetUniformLocation(particleProgram->Program, "deltaTime"), 1, &Time.deltaTime);
    glUniform1i(glGetUniformLocation(particleProgram->Program, "max_particles"), MAX_PARTICLES);
    GLuint fo = glGetUniformLocation(particleProgram->Program, "floatingOrigin");
    glUniform3f(fo, floatingOrigin.x, floatingOrigin.y, floatingOrigin.z);

    particleProgram2->use();

    glUniform1fv(glGetUniformLocation(particleProgram2->Program, "time"), 1, &t);
    glUniform1fv(glGetUniformLocation(particleProgram2->Program, "deltaTime"), 1, &Time.deltaTime);
    glUniform1i(glGetUniformLocation(particleProgram2->Program, "max_particles"), MAX_PARTICLES);
    fo = glGetUniformLocation(particleProgram2->Program, "floatingOrigin");
    glUniform3f(fo, floatingOrigin.x, floatingOrigin.y, floatingOrigin.z);

    particleProgram3->use();

    glUniform1fv(glGetUniformLocation(particleProgram3->Program, "time"), 1, &t);
    glUniform1fv(glGetUniformLocation(particleProgram3->Program, "deltaTime"), 1, &Time.deltaTime);
    glUniform1i(glGetUniformLocation(particleProgram3->Program, "max_particles"), MAX_PARTICLES);
    fo = glGetUniformLocation(particleProgram3->Program, "floatingOrigin");
    glUniform3f(fo, floatingOrigin.x, floatingOrigin.y, floatingOrigin.z);

    particleProgram3->use();

    // run program
    glUniform1ui(glGetUniformLocation(particleProgram3->Program, "count"), emitterInitCount);
    glUniform1ui(glGetUniformLocation(particleProgram3->Program, "stage"), 0);
    glDispatchCompute(emitterInitCount / 128 + 1, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    glUniform1ui(glGetUniformLocation(particleProgram3->Program, "count"), emitters_size);
    glUniform1ui(glGetUniformLocation(particleProgram3->Program, "stage"), 1);
    glDispatchCompute(emitters_size / 128 + 1, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    atomicCounters->retrieveData();
    (*atomicCounters)[1] = 0;
    (*atomicCounters)[2] = 0;
    atomicCounters->bufferData();
    // glFlush();

    vector<uint> acs = *(atomicCounters->storage);

    particleProgram2->use();
    glUniform1ui(glGetUniformLocation(particleProgram2->Program, "burstOffset"), (*atomicCounters)[0]);

    glUniform1ui(glGetUniformLocation(particleProgram2->Program, "count"), gpu_particle_bursts->size());
    glUniform1ui(glGetUniformLocation(particleProgram2->Program, "stage"), 2);
    glDispatchCompute(gpu_particle_bursts->size() / 128 + 1, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    atomicCounters->retrieveData(); // TODO // replace with dispatch indirect

    glUniform1ui(glGetUniformLocation(particleProgram2->Program, "count"), (*atomicCounters)[1]);
    glUniform1ui(glGetUniformLocation(particleProgram2->Program, "stage"), 3);
    glDispatchCompute((*atomicCounters)[1] / 128 + 1, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    particleProgram->use();
    glUniform1ui(glGetUniformLocation(particleProgram->Program, "count"), 1); // if particle emissions exceed buffer
    glUniform1ui(glGetUniformLocation(particleProgram->Program, "stage"), 4);
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    glUniform1ui(glGetUniformLocation(particleProgram->Program, "count"), 1); // reset particle counter
    glUniform1ui(glGetUniformLocation(particleProgram->Program, "stage"), 5);
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    glUniform1ui(glGetUniformLocation(particleProgram->Program, "count"), MAX_PARTICLES); // count particles
    glUniform1ui(glGetUniformLocation(particleProgram->Program, "stage"), 6);
    glDispatchCompute(MAX_PARTICLES / 256 + 1, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    atomicCounters->retrieveData();
    pcMutex.lock();
    particleCount = atomicCounters->storage->at(0);
    actualParticles = atomicCounters->storage->at(2);
    pcMutex.unlock();

    glUniform1ui(glGetUniformLocation(particleProgram->Program, "count"), actualParticles);
    glUniform1ui(glGetUniformLocation(particleProgram->Program, "stage"), 7);
    glDispatchCompute(actualParticles / 256 + 1, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    // atomicCounters->retrieveData();
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

gpu_vector<uint> *atomics = new gpu_vector<uint>();
gpu_vector_proxy<d> *_input = new gpu_vector_proxy<d>();
gpu_vector_proxy<d> *_output = new gpu_vector_proxy<d>();

#include "../textureAtlas.h"
namespace particle_renderer
{
    GLuint VAO = 0;
    glm::mat3 camInv;
    glm::vec3 camP;

    unique_ptr<Shader> particleShader;

    vector<d> res;
    ofstream output;
    rolling_buffer time;
    sorter<d> *p_sort;
    // _texture particle_tex;
    texAtlas atlas{"particleAtlas"};
    void setCamCull(glm::mat3 ci, glm::vec3 cp)
    {
        camInv = ci;
        camP = cp;
    }
    void init()
    {
        time = rolling_buffer(1000);
        output = ofstream("particle_perf.txt", ios_base::app);
        atomics->ownStorage();
        atomics->storage->push_back(0);

        if (VAO == 0)
        {
            glGenVertexArrays(1, &VAO);
        }

        _input->tryRealloc(MAX_PARTICLES);
        _output->tryRealloc(MAX_PARTICLES);

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

        _texture particle_tex;
        particle_tex.load("res/images/particle.png");

        // vector<glm::vec4> colors;
        vector<glm::u8vec4> colors;
        colors.resize(particle_tex.meta()->dims.x * particle_tex.meta()->dims.y);
        particle_tex.meta()->read(colors.data());
        for (auto &col : colors)
        {
            col.r = 255;
            col.g = 255;
            col.b = 255;
        }
        particle_tex.meta()->write(colors.data());

        atlas.addTexture(particle_tex);
        emitter_proto_asset::particleTextureAtlas = &atlas;
    }

    void init2()
    {
        particleShader = unique_ptr<Shader>(new Shader("res/shaders/particles/particles.vert", "res/shaders/particles/particles.geom", "res/shaders/particles/particles.frag"));
    }

    void end()
    {
        particleSortProgram.release();
        particleSortProgram2.release();
        particleProgram.release();
        particleProgram2.release();
        particleProgram3.release();

        output << "sort:" << time.getAverageValue() << endl;
        output.close();
    }

    void sortParticles(mat4 vp, mat4 view, vec3 camPos, vec3 camForw, vec3 camup, vec2 screen)
    {
        timer t1;
        particleSortProgram->use();

        particleSortProgram->setMat3("camInv", camInv);
        particleSortProgram->setVec3("camPos", camPos);
        particleSortProgram->setVec3("camp", camPos);
        particleSortProgram->setVec3("cameraForward", camForw);
        particleSortProgram->setVec3("cameraUp", camup);
        particleSortProgram->setFloat("x_size", screen.x);
        particleSortProgram->setFloat("y_size", screen.y);

        gpuTimer gt;
        t1.start();
        atomics->storage->at(0) = 0;
        atomics->bufferData();
        // histo->bufferData();

        livingParticles->bindData(0);
        _input->bindData(1);
        _output->bindData(2);
        // block_sums->bindData(3);
        particles->bindData(4);
        atomics->bindData(5);
        // histo->bindData(7);
        gpu_emitter_prototypes->bindData(8);
        gpu_emitters->bindData(9);
        GPU_TRANSFORMS->bindData(10);

        gt.start();
        particleSortProgram->use();

        particleSortProgram->setInt("stage", -1);
        particleSortProgram->setUint("count", actualParticles);
        glDispatchCompute(actualParticles / 256 + 1, 1, 1);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
        uint numParticles;
        atomics->retrieveData();
        numParticles = atomics->storage->at(0);
        appendStat("particle list create", gt.stop());

        gt.start();
        p_sort->sort(numParticles, _input, _output);
        appendStat("particle list sort", gt.stop());
    }

    void drawParticles(mat4 view, mat4 rot, mat4 proj, glm::vec3 camPos, float farplane, float scr_height, float scr_width)
    {

        for (auto &i : emitter_manager.meta)
        {
            rect r = atlas.uvMap.at(i.second->texture);
            emitter_prototypes_.get(i.second->ref).texCoord = r.coord;
            emitter_prototypes_.get(i.second->ref).sz = r.sz;
        }

        glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
        particleShader->use();
        particleShader->setMat4("view", view);
        // particleShader->setMat4("view",view);
        // GLuint matPView = glGetUniformLocation(particleShader.s->shader->Program, "view");
        particleShader->setMat4("vRot", rot);
        particleShader->setMat4("projection", proj);
        particleShader->setVec3("cameraPos", camPos);
        particleShader->setFloat("aspectRatio", (float)scr_width / (float)scr_height);

        particleShader->setFloat("FC", 2.0 / log2(farplane + 1));
        particleShader->setFloat("screenHeight", (float)scr_height);
        particleShader->setFloat("screenWidth", (float)scr_width);
        particleShader->setFloat("time", Time.time);
        particleShader->setMat3("camInv", camInv);

        particleShader->setInt("particle_tex", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, atlas.atlas.meta()->glid);

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
    // if (emitterInits.size() > 0)
    //     cout << "g";
    gpu_emitter_inits->bufferData(emitterInits);
    gpu_emitter_prototypes->bufferData();
    gpu_particle_bursts->bufferData();
}


// void encodeEmitters(YAML::Node &node)
// {
//     node["emitter_prototpes_"] = emitter_prototypes_;
//     // node["emitter_proto_assets"] = emitter_proto_assets;
//     YAML::Node epa;
//     for (auto &i : emitter_proto_assets)
//     {
//         epa.force_insert(i.first, *i.second);
//         // node["emitter_proto_assets"][i.first] = *i.second;
//     }
//     node["emitter_proto_assets"] = epa;
// }
// void decodeEmitters(YAML::Node &node)
// {
//     new (&emitter_prototypes_) array_heap<emitter_prototype>{node["emitter_prototpes_"].as<decltype(emitter_prototypes_)>()};
//     // emitter_proto_assets = node["emitter_proto_assets"].as<decltype(emitter_proto_assets)>();
//     YAML::Node epa = node["emitter_proto_assets"];
//     for (YAML::const_iterator i = epa.begin(); i != epa.end(); ++i)
//     {
//         emitter_proto_assets[i->first.as<int>()] = make_shared<emitter_proto_asset>(i->second.as<emitter_proto_asset>());
//     }
//     for(auto& i : emitter_proto_assets){
//         particle_renderer::atlas.addTexture(i.second->texture);
//     }
// }

void emitterManager::init(){
    for(auto& i : meta){
        particle_renderer::atlas.addTexture(i.second->texture);
    }
}
