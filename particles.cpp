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
    // vec3 position2;
    int next;
    
    // smvec3 velocity2; // 2ints
    float p1;    
    float l;
    int p2;
};

// vec4 color;
void emitter_prototype_::color(vec4 c)
{
    emitter_proto_assets[this->emitterPrototype]->gradient.keys.clear();
    emitter_proto_assets[this->emitterPrototype]->gradient.keys.emplace(0.f, colorArray::key(colorArray::idGenerator++, c));
    emitter_proto_assets[this->emitterPrototype]->gradient.setColorArray(emitter_proto_assets[this->emitterPrototype]->ref->colorLife);
    // for (int i = 0; i < 100; ++i)
    // {
    //     emitter_proto_assets[this->emitterPrototype]->ref->colorLife[i] = c;
    // }
}
void emitter_prototype_::color(colorArray &c)
{
    emitter_proto_assets[this->emitterPrototype]->gradient.keys.clear();
    for (auto &i : c.keys)
    {
        emitter_proto_assets[this->emitterPrototype]->gradient.addKey(i.second.value, i.first);
    }
    // emitter_proto_assets[this->emitterPrototype]->gradient = c;
    emitter_proto_assets[this->emitterPrototype]->gradient.setColorArray(emitter_proto_assets[this->emitterPrototype]->ref->colorLife);
    // for (int i = 0; i < 100; ++i)
    // {
    //     emitter_proto_assets[this->emitterPrototype]->ref->colorLife[i] = c;
    // }
}
void emitter_prototype_::color(vec4 c1, vec4 c2)
{
    emitter_proto_assets[this->emitterPrototype]->gradient.keys.clear();
    emitter_proto_assets[this->emitterPrototype]->gradient.addKey(c1, 0.f);
    emitter_proto_assets[this->emitterPrototype]->gradient.addKey(c2, 1.f);
    // emitter_proto_assets[this->emitterPrototype]->gradient.keys.emplace(0.f,colorArray::key(colorArray::idGenerator++,c1));
    // emitter_proto_assets[this->emitterPrototype]->gradient.keys.emplace(1.f,colorArray::key(colorArray::idGenerator++,c2));
    emitter_proto_assets[this->emitterPrototype]->gradient.setColorArray(emitter_proto_assets[this->emitterPrototype]->ref->colorLife);
    // vec4 step = (c2 - c1) / 100.f;
    // for (int i = 0; i < 100; ++i)
    // {
    //     emitter_proto_assets[this->emitterPrototype]->ref->colorLife[i] = c1 + step * (float)i;
    // }
}
void emitter_prototype_::size(float c)
{
    for (int i = 0; i < 100; ++i)
    {
        emitter_proto_assets[this->emitterPrototype]->ref->sizeLife[i] = c;
    }
}
void emitter_prototype_::size(float c1, float c2)
{
    float step = (c2 - c1) / 100.f;
    for (int i = 0; i < 100; ++i)
    {
        emitter_proto_assets[this->emitterPrototype]->ref->sizeLife[i] = c1 + step * (float)i;
    }
}

_texture gradientEdit;
deque<colorArray> colorGradients;
void addColorArray(colorArray &a)
{
    colorGradients.emplace_back(a);
}

struct gradientEditorData{
    int gradientKeyId;
    int color_picker_open;
};
unordered_map<colorArray*,gradientEditorData> gradientEditorDatas;
bool colorArrayEdit(colorArray &a, bool *p_open)
{
    gradientEditorDatas[&a].gradientKeyId = -1;
    float width = ImGui::GetWindowSize().x - 20;
    ImVec2 size = ImVec2(width, 20.0f);               // Size of the image we want to make visible
    ImVec2 uv0 = ImVec2(0.0f, 0.0f);                  // UV coordinates for lower-left
    ImVec2 uv1 = ImVec2(1.f, 1.f);                    // UV coordinates for (32,32) in our texture
    ImVec4 bg_col = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);   // Black background
    ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // No tint

    ImGuiIO &io = ImGui::GetIO();
    ImGuiStyle &style = ImGui::GetStyle();
    float radius_outer = 20.0f;
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 center = ImVec2(pos.x + radius_outer, pos.y + radius_outer);
    float line_height = ImGui::GetTextLineHeight();
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    bool changed = false;

    if (a.t.t == 0)
    {
        a.t.namedTexture("gradient" + to_string(rand()));
        a.t.t->gen(100, 1);
        vec4 colors[100];
        a.setColorArray(colors);
        a.t.t->write(&colors[0][0], GL_RGBA, GL_FLOAT);
    }

    draw_list->AddImage((void *)a.t.t->id, pos, {pos.x + size.x, pos.y + size.y}, uv0, uv1, ImGui::GetColorU32(tint_col));

    gradientEditorDatas[&a].color_picker_open = 0;
    int i = 0;
    for (auto key : a.keys)
    {

        // add carat
        float p_value = key.first * width;
        ImGui::SetCursorScreenPos({pos.x + p_value - 5, pos.y});
        bool clicked = ImGui::InvisibleButton(("pointer" + to_string(key.second.id)).c_str(), {10, 10});
        if (clicked)
        {
            gradientEditorDatas[&a].color_picker_open = i;
        }
        bool value_changed = false;
        bool is_active = ImGui::IsItemActive();
        bool is_hovered = ImGui::IsItemHovered();
        if (ImGui::IsMouseReleased)
            gradientEditorDatas[&a].gradientKeyId = -1;

        if (io.MouseDown[1] && is_hovered)
        {
            a.keys.erase(key.first);
            continue;
        }
        if (is_active && io.MouseDelta.x != 0.0f && (gradientEditorDatas[&a].gradientKeyId == -1 || key.second.id == gradientEditorDatas[&a].gradientKeyId))
        {
            p_value += io.MouseDelta.x;
            if (p_value < 0)
                p_value = 0;
            if (p_value > width - 1)
                p_value = width - 1;
            value_changed = true;
            gradientEditorDatas[&a].gradientKeyId = key.second.id;
        }
        i++;
        if (value_changed)
        {
            a.keys.erase(key.first);
            if (a.keys.find(p_value) == a.keys.end())
                a.keys.emplace(p_value / width, key.second);
            else
            {
                a.keys.emplace(p_value / width + 0.01f, key.second);
            }

            vec4 colors[100];
            a.setColorArray(colors);
            a.t.t->write(&colors[0][0], GL_RGBA, GL_FLOAT);
            changed = true;
        }
        draw_list->AddTriangleFilled({pos.x + p_value - 5, pos.y}, {pos.x + p_value, pos.y + 10}, {pos.x + p_value + 5, pos.y}, ImGui::GetColorU32(tint_col));
        draw_list->AddTriangleFilled({pos.x + p_value - 4, pos.y}, {pos.x + p_value, pos.y + 8}, {pos.x + p_value + 4, pos.y}, ImGui::GetColorU32(*(ImVec4 *)&key.second.value));
        draw_list->AddTriangle({pos.x + p_value - 5, pos.y}, {pos.x + p_value, pos.y + 10}, {pos.x + p_value + 5, pos.y}, ImGui::GetColorU32(bg_col), 2.f);
    }
    ImGui::SetCursorScreenPos(pos);
    if (gradientEditorDatas[&a].gradientKeyId == -1)
    {
        bool clicked = ImGui::InvisibleButton("gradient_click_area", size);
        if (clicked)
        {
            cout << "clicked" << endl;
            ImVec2 m_pos = io.MousePos;
            cout << "m_pos " << m_pos.x << "," << m_pos.y << endl;
            cout << "pos " << pos.x << "," << pos.y << endl;
            float key = (float)(m_pos.x - pos.x) / (float)size.x;
            a.addKey(vec4(1), key);
            gradientEditorDatas[&a].color_picker_open = 0;
            for (auto &i : a.keys)
            {
                if (i.first >= key)
                    break;
                gradientEditorDatas[&a].color_picker_open++;
            }
            // gradientKeyId = a.keys.at(key).id;
        }
    }
    ImGui::SetCursorScreenPos({pos.x, pos.y + 20 + style.ItemInnerSpacing.y});
    // if(clicked && ImGui::IsMouseClicked(2))
    //     color_picker_open = true;
    // if(color_picker_open){
    auto key = a.keys.begin();
    std::advance(key, gradientEditorDatas[&a].color_picker_open);
    if (ImGui::ColorPicker4("color_", (float *)&key->second.value))
    {
        vec4 colors[100];
        a.setColorArray(colors);
        a.t.t->write(&colors[0][0], GL_RGBA, GL_FLOAT);
        changed = true;
        // a.keys.find(key.first)->second.value = key-second.value;
    }
    // }
    return changed;
}

int colorArray::idGenerator = 0;
colorArray &colorArray::addKey(vec4 color, float position)
{
    keys.emplace(position, key(idGenerator++, color));
    return *this;
}
void colorArray::setColorArray(vec4 *colors)
{
    if (keys.size() == 0)
        return;

    auto k1 = keys.begin();
    auto k2 = k1;
    ++k2;

    int end = k1->first * 100;
    for (int i = 0; i < end; ++i)
    {
        colors[i] = k1->second.value;
    }
    while (k2 != keys.end())
    {
        int start = k1->first * 100;
        int end = k2->first * 100;
        vec4 step = (k2->second.value - k1->second.value) / (float)(end - start);
        int j = 0;
        for (int i = start; i < end; i++, j++)
        {
            colors[i] = k1->second.value + step * (float)(j);
        }
        k1 = k2++;
    }
    for (int i = k1->first * 100; i < 100; ++i)
    {
        colors[i] = k1->second.value;
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

// simulation
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
map<string, int> emitter_prototypes;
map<int, string> emitter_proto_names;
map<int, emitter_proto_asset *> emitter_proto_assets;


// renderering
array_heap<emitter> EMITTERS;
gpu_vector_proxy<emitter> *gpu_emitters = new gpu_vector_proxy<emitter>();
gpu_vector_proxy<emitterInit> *gpu_emitter_inits = new gpu_vector_proxy<emitterInit>();
vector<emitterInit> emitterInits;
vector<emitterInit> emitterInitsdb;
unordered_map<uint, emitterInit> emitter_inits;



void saveEmitters(OARCHIVE &oa)
{
    oa << emitter_prototypes_ << emitter_prototypes << emitter_proto_names << emitter_proto_assets << colorGradients;
}
void loadEmitters(IARCHIVE &ia)
{
    ia >> emitter_prototypes_ >> emitter_prototypes >> emitter_proto_names >> emitter_proto_assets >> colorGradients;
}

mutex burstLock;
void swapBurstBuffer()
{
    gpu_particle_bursts->storage->swap(particle_bursts);
    particle_bursts.clear();
}
int emitter_prototype_::getId()
{
    return emitter_proto_assets[emitterPrototype]->ref.index;
}
emitter_prototype *emitter_prototype_::operator->()
{
    return &emitter_proto_assets[emitterPrototype]->ref.data();
}
emitter_prototype &emitter_prototype_::operator*()
{
    return emitter_proto_assets[emitterPrototype]->ref.data();
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

REGISTER_ASSET(emitter_proto_asset);
emitter_prototype_ createNamedEmitter(string name)
{
    emitter_proto_asset *ep = new emitter_proto_asset();
    ep->genID();
    ep->ref = emitter_prototypes_._new();
    emitter_proto_assets[ep->id] = ep;
    emitter_prototypes.insert(std::pair<string, int>(name, ep->id));
    // ret.emitterPrototype = emitter_prototypes.at(name);
    emitter_proto_names[ep->id] = name;
    emitter_prototype_ ret;
    ret.emitterPrototype = ep->id;
    // ret.genID();
    return ret;
}
emitter_prototype_ getNamedEmitterProto(string name)
{
    emitter_prototype_ ret;
    ret.emitterPrototype = emitter_prototypes.at(name);
    return ret;
}
emitter_proto_asset *emitter_prototype_::meta()
{
    return emitter_proto_assets[emitterPrototype];
}
bool emitter_proto_asset::onEdit()
{

    // char input[1024];
    // sprintf(input, name.c_str());
    // if (ImGui::InputText("", input, 1024, ImGuiInputTextFlags_None))
    //     name = {input};
    // ImGui::PopID();
    // ImGui::PopItemWidth();
    // bool ret = ImGui::Button(name.c_str(), {40, 40});
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
    {
        // Set payload to carry the index of our item (could be anything)
        ImGui::SetDragDropPayload("EMITTER_PROTOTYPE_DRAG_AND_DROP", &this->id, sizeof(int));
        ImGui::EndDragDropSource();
    }
    // return ret;

    // emitterPrototype->edit();
}

string emitter_proto_asset::type(){
    return "EMITTER_PROTOTYPE_DRAG_AND_DROP";
}

void emitter_proto_asset::inspect()
{
    if (gradient.t.t == 0)
    {
        gradient.t.namedTexture("colOverLife" + to_string(id));
        gradient.t.t->gen(100, 1);
    }
    this->ref->edit(gradient);
}

void particle_emitter::onEdit()
{
    ImGui::InputText("prototype", (char *)emitter_proto_names.at(prototype.emitterPrototype).c_str(), emitter_proto_names.at(prototype.emitterPrototype).size() + 1, ImGuiInputTextFlags_ReadOnly);
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("EMITTER_PROTOTYPE_DRAG_AND_DROP"))
        {
            IM_ASSERT(payload->DataSize == sizeof(int));
            int payload_n = *(const int *)payload->Data;
            prototype = getNamedEmitterProto(emitter_proto_names.at(payload_n));
            if(this->transform.id != -1)
                this->setPrototype(prototype);
        }
        ImGui::EndDragDropTarget();
    }
    // RENDER(prototype);
}

void renderEdit(const char *name, emitter_prototype_ &ep)
{
    ImGui::InputText(name, (char *)emitter_proto_names.at(ep.emitterPrototype).c_str(), emitter_proto_names.at(ep.emitterPrototype).size() + 1, ImGuiInputTextFlags_ReadOnly);
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



void particle_emitter::protoSetPrototype(emitter_prototype_ ep)
{
    prototype = ep;
}

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
// void particle_emitter::onEdit()
// {
//     for (auto &i : emitter_prototypes)
//     {
//         if (this->prototype.getId() == i.second.index)
//             ImGui::Text(i.first.c_str());
//     }
// }
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
    emitter_proto_asset *ep = new emitter_proto_asset();
    ep->ref = emitter_prototypes_._new();
    emitter_proto_assets[0] = ep;

    // gpu_emitters->tryRealloc(1024 * 1024 * 4);
}

Shader particleSortProgram("res/shaders/particles/particle_sort_1.comp");
// Shader particleSortProgram2("res/shaders/particle_sort2.comp");
Shader particleSortProgram2("res/shaders/particles/particle_sort_2.comp");
Shader particleProgram("res/shaders/particles/particleUpdate_.comp");
Shader particleProgram2("res/shaders/particles/particleUpdate_burst.comp");
Shader particleProgram3("res/shaders/particles/particleUpdate_emitter.comp");
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


gpu_vector<uint> *atomics = new gpu_vector<uint>();
gpu_vector_proxy<d> *_input = new gpu_vector_proxy<d>();
gpu_vector_proxy<d> *_output = new gpu_vector_proxy<d>();


namespace particle_renderer
{
    GLuint VAO = 0;
    glm::mat3 camInv;
    glm::vec3 camP;

    _shader particleShader;


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
        atomics->ownStorage();
        atomics->storage->push_back(0);
        particleShader = _shader("res/shaders/particles/particles.vert", "res/shaders/particles/particles.geom", "res/shaders/particles/particles.frag");

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
        particleSortProgram.use();


        particleSortProgram.setInt("stage", -1);
        particleSortProgram.setUint("count", actualParticles);
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
        particleShader->setVec3("cameraPos", mainCamPos);
        particleShader->setFloat("aspectRatio", (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT);

        particleShader->setFloat("FC", 2.0 / log2(farplane + 1));
        particleShader->setFloat("screenHeight", (float)SCREEN_HEIGHT);
        particleShader->setFloat("screenWidth", (float)SCREEN_WIDTH);
        particleShader->setMat3("camInv", camInv);

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