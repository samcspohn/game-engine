#pragma once
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <vector>
#include <list>
#include <string>
// #include "_rendering/texture.h"
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include "serialize.h"
#include <unordered_map>

using namespace std;

class inspectable
{
public:
    virtual void inspect(){};
};

namespace assets
{
    class asset : public inspectable
    {
    public:
        int id = 0;
        string name;
        virtual string type() = 0;
        int genID();
        virtual void copy();
        virtual bool onEdit() = 0;
        // virtual void load() = 0;
    };
    struct assetManagerBase{
        virtual void load(string){};
    };

    template <typename t> // must be asset
    struct assetManager : assetManagerBase
    {
        map<string, int> path;
        unordered_map<int, shared_ptr<t>> meta;
        void encode(YAML::Node &node)
        {
            YAML::Node assets;
            YAML::Node meta_node;
            // YAML::Node models_node;
            assets["path"] = path;
            for (auto &i : path)
            {
                if(meta.find(i.second) != meta.end())
                    meta_node.force_insert(i.second, *meta.at(i.second));
            }
            assets["meta"] = meta_node;
            node[typeid(t).name()] = assets;
        }

        void decode(YAML::Node &node)
        {

            YAML::Node assets = node[typeid(t).name()];
            path = assets["path"].as<map<string, int>>();
            for (auto& i : path)
            {
                meta[i.second] = std::make_shared<t>(assets["meta"][to_string(i.second)].as<t>());
            }
        }

        // void _new() = 0;
    };

    template<typename t>
    struct asset_instance
    {
        int id = -1;
        virtual t *meta() const = 0;
    };
    // extern unordered_map<size_t,assetManager*>

    // extern map<int, asset*> assets;
    // void registerAsset(asset*);
    extern int assetIdGenerator;
}

template<typename t>
void AssetRenderEdit(const char* name, assets::asset_instance<t> * a){
    if (a->id == -1) // uninitialized
		ImGui::InputText(name, "", 1, ImGuiInputTextFlags_ReadOnly);
	else
		ImGui::InputText(name, (char *)a->meta()->name.c_str(), a->meta()->name.size() + 1, ImGuiInputTextFlags_ReadOnly);
}

#define YAML_ENCODE_ASSET() \
    node["id"] = rhs.id;    \
    node["name"] = rhs.name;
#define YAML_DECODE_ASSET()        \
    rhs.id = node["id"].as<int>(); \
    rhs.name = node["name"].as<string>();

// namespace YAML
// {
// 	template <>
// 	struct convert<assets::asset>
// 	{
// 		static Node encode(const assets::asset &rhs)
// 		{
// 			Node node;
// 			node["id"] = rhs.id;
//             node["name"] = rhs.name;
// 			return node;
// 		}

// 		static bool decode(const Node &node, assets::asset &rhs)
// 		{
// 			rhs.id = node["id"].as<int>();
//             rhs.name = node["name"].as<string>();
// 			return true;
// 		}
// 	};
// }

#define RENDER(name) \
    renderEdit(#name, name);

#define _RENDER(_name, name) \
    renderEdit(_name, name);

void renderEdit(const char *name, string &s);
void renderEdit(const char *name, bool &b);
void renderEdit(const char *name, int &i);
void renderEdit(const char *name, float &f);
void renderEdit(const char *name, glm::vec2 &v);
void renderEdit(const char *name, glm::vec3 &v);
void renderEdit(const char *name, glm::vec4 &v);
void renderEdit(const char *name, glm::quat &q);

template <typename t>
void renderEdit(const char *name, vector<t> &v)
{
    bool open = ImGui::TreeNode(name);
    ImGui::SameLine();
    bool add = ImGui::Button("+");
    int remove = -1;
    std::hash<string> x;
    if (open)
    {
        for (int i{0}; i < v.size(); ++i)
        {
            ImGui::PushID(x(name) + i);
            renderEdit(to_string(i).c_str(), v[i]);
            ImGui::SameLine();
            if (ImGui::Button("-"))
            {
                remove = i;
            }
            ImGui::PopID();
        }
        if (remove != -1)
        {
            v.erase(v.begin() + remove);
        }
        ImGui::TreePop();
    }
    if (add)
    {
        v.emplace_back();
    }
}

template <typename t, size_t u>
void renderEdit(const char *name, t (&v)[u])
{
    bool open = ImGui::TreeNode(name);
    if (open)
    {
        for (int i{0}; i < u; ++i)
        {
            ImGui::PushID(i);
            renderEdit(to_string(i).c_str(), v[i]);
            ImGui::PopID();
        }
        ImGui::TreePop();
    }
}

template <typename t, size_t u>
void renderEdit(const char *name, array<t, u> &v)
{
    bool open = ImGui::TreeNode(name);
    if (open)
    {
        for (int i{0}; i < u; ++i)
        {
            ImGui::PushID(i);
            renderEdit(to_string(i).c_str(), v[i]);
            ImGui::PopID();
        }
        ImGui::TreePop();
    }
}
// void renderEdit(const char* name, bool& b);