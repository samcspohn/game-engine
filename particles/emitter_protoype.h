#pragma once

#include <map>
#include <unordered_map>
#include "_serialize.h"
#include "_rendering/texture.h"
#include <glm/glm.hpp>
#include "particles/gradient.h"
#include "gpu_vector.h"
#include "array_heap.h"
// #include "particles/emitter.h"

struct _burst
{
    glm::vec3 position;
    uint emitter_prototype;
    glm::vec3 direction;
    uint count;
    glm::vec3 scale;
    int p1;
};

struct emitter_prototype
{
    float emission_rate{10};
    float lifetime{2};
    float rotation_rate{0};
    float dispersion{3.14159f};

    // vec4 color;

    float minSpeed{2};
    float maxSpeed{3};
    float lifetime2{3};
    int live;

    glm::vec2 scale{1};
    int billboard{1};
    int p3;

    int velAlign{0};
    float radius{0};
    int p2;
    int trail{0};
    array<glm::vec4, 100> colorLife;
    array<float, 100> sizeLife{1.f};
    emitter_prototype()
    {
        colorLife.fill(glm::vec4(1));
        sizeLife.fill(1.f);
    }
    SER_HELPER()
    {
        ar &emission_rate &lifetime &rotation_rate &dispersion &minSpeed
            &maxSpeed &lifetime2 &live &scale &billboard &velAlign
                &radius &trail &colorLife &sizeLife;
    }
    void edit(colorArray &ca)
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
        if (ImGui::ImageButton(ca.t.t->id, size, uv0, uv1, frame_padding, bg_col, tint_col))
        {
            col_p_open = true;
        }
        // ImGui::SetNextWindowSize({200, 200});
        // ImGui::SetNextWindowPos({ImGui::GetMainViewport()->GetCenter()});
        if (col_p_open)
        {

            ImGui::Begin("color over life", &col_p_open);

            if (colorArrayEdit(ca, &col_p_open))
            {
                ca.setColorArray(colorLife.data());
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
    int ref;
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
extern gpu_vector<emitter_prototype> *gpu_emitter_prototypes;
extern array_heap<emitter_prototype> emitter_prototypes_;
extern map<int, shared_ptr<emitter_proto_asset>> emitter_proto_assets;
extern gpu_vector<_burst> *gpu_particle_bursts;

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
    void color(glm::vec4 c);
    void color(colorArray &c);
    void color(glm::vec4 c1, glm::vec4 c2);
    void size(float c);
    void size(float c1, float c2);

    emitter_proto_asset *meta();
    int getId();
    emitter_prototype *operator->();
    emitter_prototype &operator*();
    void burst(glm::vec3 pos, glm::vec3 dir, uint count);
    void burst(glm::vec3 pos, glm::vec3 dir, glm::vec3 scale, uint count);
    // friend emitter_prototype_ createNamedEmitter(string name);
    // friend emitter_prototype_ getNamedEmitterProto(string name);
    friend emitter_prototype_ createEmitter(string name);
    SER_HELPER()
    {
        ar &emitterPrototype;
    }
    friend struct YAML::convert<emitter_prototype_>;
};

void renderEdit(string name, emitter_prototype_ &ep);
emitter_prototype_ createEmitter(string name);

void init_prototypes();

namespace YAML
{
    template <>
    struct convert<emitter_prototype_>
    {
        static Node encode(const emitter_prototype_ &rhs)
        {
            Node node;
            node["id"] = rhs.emitterPrototype;
            return node;
        }

        static bool decode(const Node &node, emitter_prototype_ &rhs)
        {
            rhs.emitterPrototype = node["id"].as<int>();
            return true;
        }
    };

    template <>
    struct convert<emitter_proto_asset>
    {
        static Node encode(const emitter_proto_asset &rhs)
        {
            Node node;
            YAML_ENCODE_ASSET();
            return node;
        }

        static bool decode(const Node &node, emitter_proto_asset &rhs)
        {
            YAML_DECODE_ASSET();
            return true;
        }
    };

    template <>
    struct convert<emitter_prototype>
    {
        static Node encode(const emitter_prototype &rhs)
        {
#define ENCODE_PROTO(arg) node[#arg] = rhs.arg;
            Node node;
            //  ar &emission_rate &lifetime &rotation_rate &dispersion &minSpeed
            // &maxSpeed &lifetime2 &live &scale &billboard &velAlign
            //     &radius &trail &colorLife &sizeLife;
            // node["id"] = rhs.emitterPrototype;
            ENCODE_PROTO(emission_rate);
            ENCODE_PROTO(lifetime);
            ENCODE_PROTO(rotation_rate);
            ENCODE_PROTO(dispersion);
            ENCODE_PROTO(minSpeed);
            ENCODE_PROTO(maxSpeed);
            ENCODE_PROTO(lifetime2);
            ENCODE_PROTO(live);
            ENCODE_PROTO(scale);
            ENCODE_PROTO(billboard);
            ENCODE_PROTO(velAlign);
            ENCODE_PROTO(radius);
            ENCODE_PROTO(trail);
            ENCODE_PROTO(colorLife);
            ENCODE_PROTO(sizeLife);
        #undef ENCODE_PROTO
            return node;
        }

        static bool decode(const Node &node, emitter_prototype &rhs)
        {
            // rhs.emitterPrototype = node["id"].as<int>();

            #define DECODE_PROTO(arg) rhs.arg = node[#arg].as<decltype(rhs.arg)>();
            //  ar &emission_rate &lifetime &rotation_rate &dispersion &minSpeed
            // &maxSpeed &lifetime2 &live &scale &billboard &velAlign
            //     &radius &trail &colorLife &sizeLife;
            // node["id"] = rhs.emitterPrototype;
            DECODE_PROTO(emission_rate);
            DECODE_PROTO(lifetime);
            DECODE_PROTO(rotation_rate);
            DECODE_PROTO(dispersion);
            DECODE_PROTO(minSpeed);
            DECODE_PROTO(maxSpeed);
            DECODE_PROTO(lifetime2);
            DECODE_PROTO(live);
            DECODE_PROTO(scale);
            DECODE_PROTO(billboard);
            DECODE_PROTO(velAlign);
            DECODE_PROTO(radius);
            DECODE_PROTO(trail);
            DECODE_PROTO(colorLife);
            DECODE_PROTO(sizeLife);
        #undef DECODE_PROTO
            return true;
        }
    };

}