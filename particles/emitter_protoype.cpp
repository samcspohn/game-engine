
#include "emitter_protoype.h"
#include "array_heap.h"
#include "gpu_vector.h"

array_heap<emitter_prototype> emitter_prototypes_;
gpu_vector<emitter_prototype> *gpu_emitter_prototypes = new gpu_vector<emitter_prototype>();
gpu_vector<_burst> *gpu_particle_bursts = new gpu_vector<_burst>();
vector<_burst> particle_bursts;

mutex burstLock;

texAtlas* emitter_proto_asset::particleTextureAtlas = 0;
void swapBurstBuffer()
{
    gpu_particle_bursts->storage->swap(particle_bursts);
    particle_bursts.clear();
}

void emitter_prototype_::color(glm::vec4 c)
{
    
    meta()->gradient.keys.clear();
    meta()->gradient.keys.emplace(0.f, colorArray::key(colorArray::idGenerator++, c));
    meta()->gradient.setColorArray(emitter_prototypes_.get(meta()->ref).colorLife.data());
    // for (int i = 0; i < 100; ++i)
    // {
    //     meta()->ref->colorLife[i] = c;
    // }
}
void emitter_prototype_::color(colorArray &c)
{
    meta()->gradient.keys.clear();
    for (auto &i : c.keys)
    {
        meta()->gradient.addKey(i.second.value, i.first);
    }
    // meta()->gradient = c;
    meta()->gradient.setColorArray(emitter_prototypes_.get(meta()->ref).colorLife.data());
    // for (int i = 0; i < 100; ++i)
    // {
    //     meta()->ref->colorLife[i] = c;
    // }
}
void emitter_prototype_::color(glm::vec4 c1, glm::vec4 c2)
{
    meta()->gradient.keys.clear();
    meta()->gradient.addKey(c1, 0.f);
    meta()->gradient.addKey(c2, 1.f);
    // meta()->gradient.keys.emplace(0.f,colorArray::key(colorArray::idGenerator++,c1));
    // meta()->gradient.keys.emplace(1.f,colorArray::key(colorArray::idGenerator++,c2));
    meta()->gradient.setColorArray(emitter_prototypes_.get(meta()->ref).colorLife.data());
    // vec4 step = (c2 - c1) / 100.f;
    // for (int i = 0; i < 100; ++i)
    // {
    //     meta()->ref->colorLife[i] = c1 + step * (float)i;
    // }
}
void emitter_prototype_::size(float c)
{
    for (int i = 0; i < 100; ++i)
    {
        emitter_prototypes_.get(meta()->ref).sizeLife[i] = c;
    }
}
void emitter_prototype_::size(float c1, float c2)
{
    float step = (c2 - c1) / 100.f;
    for (int i = 0; i < 100; ++i)
    {
        emitter_prototypes_.get(meta()->ref).sizeLife[i] = c1 + step * (float)i;
    }
}

int emitter_prototype_::getRef()
{
    return emitter_manager.meta[e]->ref;
}
emitter_prototype *emitter_prototype_::operator->()
{
    return &emitter_prototypes_.get(meta()->ref);
}
emitter_prototype &emitter_prototype_::operator*()
{
    return emitter_prototypes_.get(meta()->ref);
}
void emitter_prototype_::burst(glm::vec3 pos, glm::vec3 dir, uint count)
{
    _burst b;
    b.direction = dir;
    b.count = count;
    b.position = pos;
    b.scale = glm::vec3(1);
    b.emitter_prototype = getRef();
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
    b.emitter_prototype = getRef();
    burstLock.lock();
    particle_bursts.push_back(b);
    burstLock.unlock();
}
emitter_prototype_ createEmitter(string name)
{
    shared_ptr<emitter_proto_asset> ep = make_shared<emitter_proto_asset>();
    ep->genID();
    ep->ref = emitter_prototypes_._new();
    emitter_manager.meta[ep->id] = ep;
    emitter_manager.path[name] = ep->id;
    ep->name = name;
    emitter_prototype_ ret;
    ret.e = ep->id;
    // ret.genID();
    return ret;
}

void emitterManager::_new(){
    shared_ptr<emitter_proto_asset> ep = make_shared<emitter_proto_asset>();
    ep->genID();
    ep->name = "emitter " + to_string(emitter_manager.meta.size()) + ".pemt";
    ep->ref = emitter_prototypes_._new();
    emitter_manager.meta[ep->id] = ep;
    emitter_manager.path[ep->name] = ep->id;
}

emitter_proto_asset::emitter_proto_asset()
{
    waitForRenderJob([&]()
                     { texture.load("res/images/particle.png"); })
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
    string s = this->name.substr(0,this->name.find_last_of('.')) + " copy";
    emitter_prototype_ cp = createEmitter(s + ".pemt");
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
    if (gradient.t.meta() == 0)
    {
        gradient.t._new();// namedTexture("colOverLife" + to_string(id));
        gradient.t.meta()->gen(100, 1);
    }
    emitter_prototypes_.get(this->ref).edit(gradient);

    auto p = texture.meta();
    renderEdit("texture", texture);
    if(p != texture.meta()){
        particleTextureAtlas->addTexture(texture);
    }
}

void renderEdit(const char *name, emitter_prototype_ &ep)
{
    if (ep.e != -1)
        ImGui::InputText(name, (char *)ep.meta()->name.c_str(), ep.meta()->name.size() + 1, ImGuiInputTextFlags_ReadOnly);
    else
        ImGui::InputText(name, "", 1, ImGuiInputTextFlags_ReadOnly);
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("EMITTER_PROTOTYPE_DRAG_AND_DROP"))
        {
            IM_ASSERT(payload->DataSize == sizeof(int));
            int payload_n = *(const int *)payload->Data;
            ep.e = payload_n;
        }
        ImGui::EndDragDropTarget();
    }
}

emitterManager emitter_manager;

void init_prototypes()
{
    gpu_particle_bursts->ownStorage();
}