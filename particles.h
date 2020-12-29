#pragma once
#include "fast_list.h"
#include <map>
#include <unordered_map>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "game_object.h"
#include "editor.h"
#include <fstream>
using namespace std;
using namespace glm;

void saveEmitters(OARCHIVE &oa);
void loadEmitters(IARCHIVE &ia);

class component;
class particle_emitter;

struct colorArray
{
    struct key
    {
        vec4 color;
        float pos;
    };
    vector<key> keys;
    colorArray &addKey(vec4 color, float position);
    void setColorArray(vec4 *colors);
};
struct floatArray
{
    struct key
    {
        float value;
        float pos;
    };
    vector<key> keys;
    floatArray &addKey(float v, float position);
    void setFloatArray(float *floats);
};
struct emitter_prototype
{
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
    SER_HELPER()
    {
        ar &emission_rate &lifetime &rotation_rate &dispersion &minSpeed
            &maxSpeed &lifetime2 &live &scale &billboard &velAlign
                &radius &trail &colorLife &sizeLife;
    }
    void edit()
    {
        RENDER(emission_rate);
        RENDER(lifetime);
        RENDER(lifetime2);
        RENDER(rotation_rate);
        RENDER(dispersion);
        RENDER(minSpeed);
        RENDER(maxSpeed);
        RENDER(scale);
        // RENDER(billboard);
        bool b = billboard;
        if(ImGui::Checkbox("billboard",&b))
            billboard = b;
        b = velAlign;
        if(ImGui::Checkbox("align to velocity",&b))
            velAlign = b;
        // RENDER(velAlign);

        RENDER(radius);
    }
};

class emitter_proto_asset : public assets::asset
{
public:
    typename array_heap<emitter_prototype>::ref ref;
    bool onEdit();
    void inspect();
    SER_HELPER()
    {
        SER_BASE_ASSET
        ar &ref;
    }
};
extern map<int, emitter_proto_asset *> emitter_proto_assets;

class emitter_prototype_
{
    int emitterPrototype;
    // typename array_heap<emitter_prototype>::ref emitterPrototype;
    // string name;
    friend emitter_prototype_ createNamedEmitter(string name);
    friend emitter_prototype_ getNamedEmitterProto(string name);
    friend void renderEdit(const char *name, emitter_prototype_ &ep);
    friend class particle_emitter;

public:
    emitter_proto_asset *meta();
    int getId();
    emitter_prototype *operator->();
    emitter_prototype &operator*();
    void burst(glm::vec3 pos, glm::vec3 dir, uint count);
    void burst(glm::vec3 pos, glm::vec3 dir, glm::vec3 scale, uint count);
    friend emitter_prototype_ createNamedEmitter(string name);
    friend emitter_prototype_ getNamedEmitterProto(string name);
    SER_HELPER()
    {
        ar &emitterPrototype;
    }
};

void renderEdit(string name, emitter_prototype_ &ep);
emitter_prototype_ createNamedEmitter(string name);
emitter_prototype_ getNamedEmitterProto(string name);

struct emitterInit
{
    uint transformID;
    uint emitterProtoID;
    int live;
    int id;
};
struct emitter
{
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
class particle_emitter final : public component
{
    emitter_prototype_ prototype;
    static mutex lock;

public:
    typename array_heap<emitter>::ref emitter;
    // typename array_heap<GLint>::ref emitter_last_particle;
    void onEdit();
    COPY(particle_emitter);
    void setPrototype(emitter_prototype_ ep);
    void protoSetPrototype(emitter_prototype_ ep);
    void onStart();
    void onDestroy();
    SER1(prototype);
};
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
}; // namespace particle_renderer

void prepParticles();
void swapBurstBuffer();
int getParticleCount();