#pragma once
// #include "fast_list.h"
#include <map>
#include <unordered_map>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include "components/game_object.h"
#include "_serialize.h"
#include "particles/emitter_protoype.h"
#include "particles/emitter.h"

using namespace std;
using namespace glm;

// void encodeEmitters(YAML::Node &node);
// void decodeEmitters(YAML::Node &node);


extern int particleCount;
extern int actualParticles;
void initParticles();
void initParticles2();
void updateParticles(vec3 floatingOrigin, uint emitterInitCount);

namespace particle_renderer
{
    void setCamCull(glm::mat3 ci, glm::vec3 cp);
    void init();
    void init2();

    void end();

    void sortParticles(mat4 vp, mat4 view, vec3 camPos, vec3 camForw, vec3 camup, vec2 screen, barrier& b);

    void drawParticles(mat4 view, mat4 rot, mat4 proj, glm::vec3 camPos, float farplane, float scr_height, float scr_width);
}; // namespace particle_renderer

void prepParticles();
void swapBurstBuffer();
int getParticleCount();
int getActualParticles();