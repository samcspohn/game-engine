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
    struct key{
        int id;
        vec4 value;
        SER_HELPER(){
            ar & id & value;
        }
        key(int _id, vec4 _value) : id(_id), value(_value) {

        }
        key() = default;
    };
    static int idGenerator;
    _texture t;
    map<float,key> keys;
    colorArray &addKey(vec4 color, float position);
    void setColorArray(vec4 *colors);
    SER_HELPER(){
        ar & keys;
    }
};

extern deque<colorArray> colorGradients;
extern int gradient_index;
void addColorArray(colorArray& c);
extern _texture gradientEdit;
bool colorArrayEdit(colorArray &a, bool *p_open);

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
    void edit(colorArray& ca)
    {
        RENDER(emission_rate);
        RENDER(lifetime);
        RENDER(lifetime2);
        RENDER(rotation_rate);
        RENDER(dispersion);
        RENDER(minSpeed);
        RENDER(maxSpeed);
        RENDER(scale);
        ImGui::ColorPicker4("color", (float *)&colorLife[0]);
        ca.t.t->write(&colorLife[0][0], GL_RGBA, GL_FLOAT);
        int frame_padding = -1;                           // -1 == uses default padding (style.FramePadding)
        ImVec2 size = ImVec2(200.0f, 20.0f);              // Size of the image we want to make visible
        ImVec2 uv0 = ImVec2(0.0f, 0.0f);                  // UV coordinates for lower-left
        ImVec2 uv1 = ImVec2(1.f, 1.f);                    // UV coordinates for (32,32) in our texture
        ImVec4 bg_col = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);   // Black background
        ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // No tint
        static bool col_p_open = false;
        if (ImGui::ImageButton((void *)ca.t.t->id, size, uv0, uv1, frame_padding, bg_col, tint_col))
        {
            col_p_open = true;
        }
        // ImGui::SetNextWindowSize({200, 200});
        // ImGui::SetNextWindowPos({ImGui::GetMainViewport()->GetCenter()});
        if (col_p_open)
        {

            ImGui::Begin("color over life", &col_p_open);

            if(colorArrayEdit(ca,&col_p_open)){
                ca.setColorArray(colorLife);
            }

            ImGui::Text("color over life");
            // draw_list->AddText(ImVec2(pos.x, pos.y + 20 + style.ItemInnerSpacing.y), ImGui::GetColorU32(ImGuiCol_Text), "color over life");

            // ImGui::SetCursorScreenPos({pos.x, pos.y + 20 + ImGui::GetFontSize() + style.ItemInnerSpacing.y * 2});
            if (ImGui::Button("close"))
            {
                col_p_open = false;
            }
            ImGui::SetItemDefaultFocus();
            ImGui::End();
        }

        // RENDER(billboard);
        bool b = billboard;
        if (ImGui::Checkbox("billboard", &b))
            billboard = b;
        b = velAlign;
        if (ImGui::Checkbox("align to velocity", &b))
            velAlign = b;
        // RENDER(velAlign);

        RENDER(radius);
    }
};

class emitter_proto_asset : public assets::asset
{
public:
    typename array_heap<emitter_prototype>::ref ref;
    colorArray gradient;
    bool onEdit();
    void copy();
    string type();
    void inspect();
    SER_HELPER()
    {
        SER_BASE_ASSET
        ar &ref &gradient;
    }
};
extern map<int, emitter_proto_asset *> emitter_proto_assets;

class emitter_prototype_
{
    int emitterPrototype = 0;
    // typename array_heap<emitter_prototype>::ref emitterPrototype;
    // string name;
    friend emitter_prototype_ createNamedEmitter(string name);
    friend emitter_prototype_ getNamedEmitterProto(string name);
    friend void renderEdit(const char *name, emitter_prototype_ &ep);
    friend class particle_emitter;

public:
    void color(vec4 c);
    void color(colorArray& c);
    void color(vec4 c1, vec4 c2);
    void size(float c);
    void size(float c1, float c2);

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
int getActualParticles();