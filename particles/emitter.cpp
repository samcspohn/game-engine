#include "particles/emitter.h"

gpu_vector_proxy<emitter> *gpu_emitters = new gpu_vector_proxy<emitter>();
gpu_vector_proxy<emitterInit> *gpu_emitter_inits = new gpu_vector_proxy<emitterInit>();
vector<emitterInit> emitterInits;
vector<emitterInit> emitterInitsdb;
unordered_map<uint, emitterInit> emitter_inits;



mutex particle_emitter::lock;
void particle_emitter::protoSetPrototype(emitter_prototype_ ep)
{
    prototype = ep;
}

void particle_emitter::setPrototype(emitter_prototype_ ep)
{
    prototype = ep;

    emitterInit ei;
    ei.emitterProtoID = prototype.getId();
    ei.live = 1;
    ei.transformID = transform.id;
    ei.id = this->id;
    lock.lock();
    emitter_inits[ei.id] = ei;
    lock.unlock();
}
void particle_emitter::onEdit()
{
    ImGui::InputText("prototype", (char *)emitter_proto_assets.at(prototype.emitterPrototype)->name.c_str(), emitter_proto_assets.at(prototype.emitterPrototype)->name.size() + 1, ImGuiInputTextFlags_ReadOnly);
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("EMITTER_PROTOTYPE_DRAG_AND_DROP"))
        {
            IM_ASSERT(payload->DataSize == sizeof(int));
            int payload_n = *(const int *)payload->Data;
            // prototype = getNamedEmitterProto(emitter_proto_names.at(payload_n));
            prototype.emitterPrototype = emitter_proto_assets.at(prototype.emitterPrototype)->id;
            if (this->transform.id != -1)
                this->setPrototype(prototype);
        }
        ImGui::EndDragDropTarget();
    }
    // RENDER(prototype);
}

void particle_emitter::init(int id)
{

    emitterInit ei;
    ei.emitterProtoID = prototype.getId();
    ei.live = 1;
    ei.transformID = transform.id;
    this->id = id;

    ei.id = this->id;
    lock.lock();
    emitter_inits[ei.id] = ei;
    lock.unlock();
}
void particle_emitter::deinit(int id)
{
    emitterInit ei;
    ei.emitterProtoID = prototype.getId();
    ei.live = 0;
    ei.transformID = transform.id;
    ei.id = this->id;
    lock.lock();
    emitter_inits[ei.id] = ei;
    lock.unlock();
    // EMITTERS._delete(this->emitter);
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