#pragma once

#include <map>
#include <unordered_map>
#include <vector>
#include "_serialize.h"
#include "_rendering/texture.h"
#include <glm/glm.hpp>
#include "particles/gradient.h"
#include "particles/emitter_protoype.h"
#include <mutex>
#include "components/Component.h"
#include "components/game_object.h"
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

    glm::vec2 p;
    int last;
    int frame;
    glm::vec4 prevPos;
};
extern vector<emitterInit> emitterInits;
extern vector<emitterInit> emitterInitsdb;
extern unordered_map<uint, emitterInit> emitter_inits;
class particle_emitter final : public component
{
    emitter_prototype_ prototype;
    static mutex lock;

public:
    int id;
    // typename array_heap<GLint>::ref emitter_last_particle;
    void onEdit();
    void setPrototype(emitter_prototype_ ep);
    void protoSetPrototype(emitter_prototype_ ep);
    void init(int id);
    void deinit(int id);
    // void onStart();
    // void onDestroy();
    // SER1(prototype);
    SER_FUNC(){

        int proto_id = prototype.emitterPrototype;
        SER(prototype);
        if (this->transform.id != -1 && prototype.emitterPrototype != proto_id)
                this->setPrototype(prototype);
    }
};

extern gpu_vector_proxy<emitter> *gpu_emitters;
extern gpu_vector_proxy<emitterInit> *gpu_emitter_inits;
extern vector<emitterInit> emitterInits;
extern vector<emitterInit> emitterInitsdb;
extern unordered_map<uint, emitterInit> emitter_inits;