
#include "emitter_protoype.h"
#include "array_heap.h"
#include "gpu_vector.h"

array_heap<emitter_prototype> emitter_prototypes_;
gpu_vector<emitter_prototype> *gpu_emitter_prototypes = new gpu_vector<emitter_prototype>();
map<int, shared_ptr<emitter_proto_asset>> emitter_proto_assets;
gpu_vector<_burst> *gpu_particle_bursts = new gpu_vector<_burst>();
vector<_burst> particle_bursts;

mutex burstLock;
void swapBurstBuffer()
{
    gpu_particle_bursts->storage->swap(particle_bursts);
    particle_bursts.clear();
}

void emitter_prototype_::color(glm::vec4 c)
{
    emitter_proto_assets[this->emitterPrototype]->gradient.keys.clear();
    emitter_proto_assets[this->emitterPrototype]->gradient.keys.emplace(0.f, colorArray::key(colorArray::idGenerator++, c));
    emitter_proto_assets[this->emitterPrototype]->gradient.setColorArray(emitter_prototypes_.get(emitter_proto_assets[this->emitterPrototype]->ref).colorLife.data());
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
    emitter_proto_assets[this->emitterPrototype]->gradient.setColorArray(emitter_prototypes_.get(emitter_proto_assets[this->emitterPrototype]->ref).colorLife.data());
    // for (int i = 0; i < 100; ++i)
    // {
    //     emitter_proto_assets[this->emitterPrototype]->ref->colorLife[i] = c;
    // }
}
void emitter_prototype_::color(glm::vec4 c1, glm::vec4 c2)
{
    emitter_proto_assets[this->emitterPrototype]->gradient.keys.clear();
    emitter_proto_assets[this->emitterPrototype]->gradient.addKey(c1, 0.f);
    emitter_proto_assets[this->emitterPrototype]->gradient.addKey(c2, 1.f);
    // emitter_proto_assets[this->emitterPrototype]->gradient.keys.emplace(0.f,colorArray::key(colorArray::idGenerator++,c1));
    // emitter_proto_assets[this->emitterPrototype]->gradient.keys.emplace(1.f,colorArray::key(colorArray::idGenerator++,c2));
    emitter_proto_assets[this->emitterPrototype]->gradient.setColorArray(emitter_prototypes_.get(emitter_proto_assets[this->emitterPrototype]->ref).colorLife.data());
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
        emitter_prototypes_.get(emitter_proto_assets[this->emitterPrototype]->ref).sizeLife[i] = c;
    }
}
void emitter_prototype_::size(float c1, float c2)
{
    float step = (c2 - c1) / 100.f;
    for (int i = 0; i < 100; ++i)
    {
        emitter_prototypes_.get(emitter_proto_assets[this->emitterPrototype]->ref).sizeLife[i] = c1 + step * (float)i;
    }
}

int emitter_prototype_::getId()
{
    return emitter_proto_assets[emitterPrototype]->ref;
}
emitter_prototype *emitter_prototype_::operator->()
{
    return &emitter_prototypes_.get(emitter_proto_assets[this->emitterPrototype]->ref);
}
emitter_prototype &emitter_prototype_::operator*()
{
    return emitter_prototypes_.get(emitter_proto_assets[this->emitterPrototype]->ref);
}
void emitter_prototype_::burst(glm::vec3 pos, glm::vec3 dir, uint count)
{
    _burst b;
    b.direction = dir;
    b.count = count;
    b.position = pos;
    b.scale = glm::vec3(1);
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


emitter_prototype_ createEmitter(string name)
{
    shared_ptr<emitter_proto_asset> ep = make_shared<emitter_proto_asset>();
    ep->genID();
    ep->ref = emitter_prototypes_._new();
    emitter_proto_assets[ep->id] = ep;
    ep->name = name;
    emitter_prototype_ ret;
    ret.emitterPrototype = ep->id;
    // ret.genID();
    return ret;
}
// emitter_prototype_ getNamedEmitterProto(string name)
// {
//     emitter_prototype_ ret;
//     ret.emitterPrototype = emitter_prototypes.at(name);
//     return ret;
// }
emitter_proto_asset *emitter_prototype_::meta()
{
    return emitter_proto_assets[emitterPrototype].get();
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
    return false;
}
void emitter_proto_asset::copy()
{
    emitter_prototype_ cp = createEmitter(this->name + " copy");
    // cp.meta()->name = cp.meta()->name;
    // assets::registerAsset(cp.meta());
    cp.meta()->gradient = this->gradient;
    emitter_prototypes_.get(cp.meta()->ref) = emitter_prototypes_.get(this->ref);
    // *(cp.meta()->ref) = *(this->ref);
}

string emitter_proto_asset::type()
{
    return "EMITTER_PROTOTYPE_DRAG_AND_DROP";
}

void emitter_proto_asset::inspect()
{
    if (gradient.t.t == 0)
    {
        gradient.t.namedTexture("colOverLife" + to_string(id));
        gradient.t.t->gen(100, 1);
    }
    emitter_prototypes_.get(this->ref).edit(gradient);
    renderEdit("texture", texture);
}



void renderEdit(const char *name, emitter_prototype_ &ep)
{
    if (ep.emitterPrototype != -1)
        ImGui::InputText(name, (char *)emitter_proto_assets.at(ep.emitterPrototype)->name.c_str(), emitter_proto_assets.at(ep.emitterPrototype)->name.size() + 1, ImGuiInputTextFlags_ReadOnly);
    else
        ImGui::InputText(name, "", 1, ImGuiInputTextFlags_ReadOnly);
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("EMITTER_PROTOTYPE_DRAG_AND_DROP"))
        {
            IM_ASSERT(payload->DataSize == sizeof(int));
            int payload_n = *(const int *)payload->Data;
            ep.emitterPrototype = emitter_proto_assets.at(payload_n)->id;
        }
        ImGui::EndDragDropTarget();
    }
}

void init_prototypes(){
    gpu_particle_bursts->ownStorage();
}