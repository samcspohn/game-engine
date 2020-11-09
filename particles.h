#pragma once
#include "fast_list.h"
#include <map>
#include <unordered_map>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "game_object.h"

#include <fstream>
using namespace std;
using namespace glm;



class component;
struct colorArray{
    struct key{
        vec4 color;
        float pos;
    };
    vector<key> keys;
    colorArray& addKey(vec4 color, float position);
    void setColorArray(vec4 *colors);
};
struct floatArray{
    struct key{
        float value;
        float pos;
    };
    vector<key> keys;
    floatArray& addKey(float v, float position);
    void setFloatArray(float *floats);
};
struct emitter_prototype{
    float emission_rate;
    float lifetime;
    float rotation_rate;
    float dispersion;

    // vec4 color;
    void color(vec4 c);
    void color(vec4 c1, vec4 c2);
    void size(float c);
    void size(float c1, float c2);

    float minSpeed;
    float maxSpeed;
    float lifetime2;
    int live;

    vec2 scale;
    int billboard;
    int p3;
    
    int velAlign;
    float radius;
    int p2;
    int trail;
    vec4 colorLife[100];
    float sizeLife[100];
};
class emitter_prototype_
{
    typename array_heap<emitter_prototype>::ref emitterPrototype;
    friend emitter_prototype_ createNamedEmitter(string name);
    friend emitter_prototype_ getEmitterPrototypeByName(string name);
public:
    uint getId();
    emitter_prototype *operator->();
    emitter_prototype &operator*();
    void burst(glm::vec3 pos, glm::vec3 dir, uint count);
    void burst(glm::vec3 pos, glm::vec3 dir,glm::vec3 scale, uint count);
    friend emitter_prototype_ createNamedEmitter(string name);
    friend emitter_prototype_ getEmitterPrototypeByName(string name);
};
emitter_prototype_ createNamedEmitter(string name);
emitter_prototype_ getEmitterPrototypeByName(string name);

struct emitterInit
{
    uint transformID;
    uint emitterProtoID;
    int live;
    int id;
};
struct emitter{
    uint transform;
    uint emitter_prototype;
    float emission;
    int live;

    vec2 p;
    int last;
    int frame;
};
extern vector<emitterInit> emitterInits;
extern vector<emitterInit> emitterInitsdb;
extern unordered_map<uint, emitterInit> emitter_inits;
class particle_emitter : public component
{
    emitter_prototype_ prototype;
    static mutex lock;

public:
    typename array_heap<emitter>::ref emitter;
    typename array_heap<GLint>::ref emitter_last_particle;
    COPY(particle_emitter);
    void setPrototype(emitter_prototype_ ep);
    void onStart();
    void onDestroy();
    SERIALIZE_CLASS(particle_emitter) SCE;
};
SERIALIZE_STREAM(particle_emitter) SSE;
extern int particleCount;
extern int actualParticles;
void initParticles();

void updateParticles(vec3 floatingOrigin, uint emitterInitCount);

namespace particle_renderer
{
    void setCamCull(glm::mat3 ci, glm::vec3 cp);
    void init();

    void end();
  
    void sortParticles(mat4 vp, mat4 view, vec3 camPos, vec2 screen);

    void drawParticles(mat4 view, mat4 rot, mat4 proj);
};

void prepParticles();
void swapBurstBuffer();
int getParticleCount();