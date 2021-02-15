#include "editor.h"
#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <string>
#include <vector>
#include <iostream>
namespace assets
{

    int assetIdGenerator{1};
    // bool asset::onEdit(){return false;}
    // string asset::type(){return "asset";}
    int asset::genID()
    {
        if (this->id == 0)
            this->id = assetIdGenerator++;
        return id;
    }
    void asset::copy()
    {
    }
    map<int, asset *> assets;
    void registerAsset(asset *ass)
    {
        assets[ass->id] = ass;
    }
    void save(OARCHIVE &oa)
    {
        // oa << assets.size();
        // for(auto& i : assets){
        //     oa << i.first;
        //     oa << i.second;
        //     // i.second->serialize(oa,0);
        // }
        oa << assets << assetIdGenerator;
    }
    void load(IARCHIVE &ia)
    {
        // size_t size;
        // ia >> size;
        // for(int i = 0; i < size; ++i){
        //     int id;
        //     asset* a;
        //     ia >> id;
        //     ia >> a;
        //     assets.emplace(pair<int,asset*>(id,a));
        // }
        ia >> assets >> assetIdGenerator;
    }
} // namespace assets
REGISTER_BASE(inspectable);
REGISTER_BASE(assets::asset)

void renderEdit(const char *name, bool &b)
{
    ImGui::Checkbox(name, &b);
}
void renderEdit(const char *name, std::string &s)
{
    vector<char> t(s.length() + 2);
    // cout << "s: " << s.size() << ": " << s.length() << endl;
    sprintf(t.data(), s.c_str());
    if (ImGui::InputText(name, t.data(), t.size()))
    {
        s = string{t.data()};
    }
}

void renderEdit(const char *name, int &i)
{
    ImGui::DragInt(name, &i);
}
void renderEdit(const char *name, float &f)
{
    ImGui::DragFloat(name, &f);
}
void renderEdit(const char *name, glm::vec2 &v)
{
    ImGui::DragFloat2(name, &v.x);
}
void renderEdit(const char *name, glm::vec3 &v)
{
    ImGui::DragFloat3(name, &v.x);
}
void renderEdit(const char *name, glm::vec4 &v)
{
    ImGui::DragFloat4(name, &v.x);
}
void renderEdit(const char *name, glm::quat &q)
{

    // glm::vec3 forw = glm::normalize(q * glm::vec3(0,0,1));
    // glm::vec3 up = glm::normalize(q * glm::vec3(0,1,0));

    // glm::vec3 realUp = glm::normalize(glm::cross(forw,glm::vec3{0,1,0}));
    // realUp = glm::cross(forw,realUp);

    // glm::vec3 angles;
    // angles.z = 0;//glm::acos(glm::dot(up + vec3(1e-20),realUp));
    // angles.y = glm::acos(glm::dot(glm::normalize(glm::vec3{forw.x,0,forw.z}),glm::vec3{0,0,1}));
    // forw = glm::rotate(forw,angles.y,glm::vec3{0,1,0});
    // angles.x = glm::acos(glm::dot(glm::normalize(forw),glm::vec3{0,0,1}));

    glm::vec3 angles = glm::eulerAngles(q);
    glm::vec3 offset = angles;
    angles = glm::degrees(angles);
    if (ImGui::DragFloat3(name, &angles.x))
    {
        angles = glm::radians(angles);
        // q = angles;

        q = glm::rotate(q, angles.x - offset.x, glm::vec3(1, 0, 0));
        q = glm::rotate(q, angles.y - offset.y, glm::vec3(0, 1, 0));
        q = glm::rotate(q, angles.z - offset.z, glm::vec3(0, 0, 1));
        // q = angles;
    }

    // glm::vec3 euler{glm::eulerAngles(q)};
    // if(ImGui::DragFloat3(name,&euler.x,0.01)){

    //     if(euler.y > glm::pi<float>() / 2)
    //         euler.y -= glm::pi<float>();
    //     else if(euler.y < -glm::pi<float>() / 2)
    //         euler.y += glm::pi<float>();

    //     if(euler.x < 0)
    //         euler.x += glm::pi<float>();
    //     // // else if(euler.x > glm::pi<float>())
    //     // //     euler.x -= glm::pi<float>();

    //     if(euler.z < 0)
    //         euler.z += glm::pi<float>();
    //     // // else if(euler.z > glm::pi<float>())
    //     // //     euler.z -= glm::pi<float>();

    //     q = euler;
    // }
    // glm::quat _q = q;
    //  if(ImGui::DragFloat4(name,&_q.x,0.01)){
    //      q = normalize(_q);
    //  }
}
