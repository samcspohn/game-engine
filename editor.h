#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <vector>
#include <list>
#include <string>
#include "texture.h"
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>


using namespace std;


#define RENDER(name)\
    renderEdit("name",name);

void renderEdit(const char* name, int& i){
    ImGui::DragInt(name,&i);
}
void renderEdit(const char* name, float& f){
    ImGui::DragFloat(name,&f);
}
void renderEdit(const char* name, glm::vec2& v){
    ImGui::DragFloat2(name,&v.x);
}
void renderEdit(const char* name, glm::vec3& v){
    ImGui::DragFloat3(name,&v.x);
}
void renderEdit(const char* name, glm::vec4& v){
    ImGui::DragFloat4(name,&v.x);
}
void renderEdit(const char* name, glm::quat& q){

    // glm::vec3 forw = glm::normalize(q * glm::vec3(0,0,1));
    // glm::vec3 up = glm::normalize(q * glm::vec3(0,1,0));

    // glm::vec3 realUp = glm::normalize(glm::cross(forw,glm::vec3{0,1,0}));
    // realUp = glm::cross(forw,realUp);

    // glm::vec3 angles;
    // angles.z = glm::acos(glm::dot(up,realUp));
    // angles.y = glm::acos(glm::dot(glm::normalize(glm::vec3{forw.x,0,forw.z}),glm::vec3{0,0,1}));
    // forw = glm::rotate(forw,angles.y,glm::vec3{0,1,0});
    // angles.x = glm::acos(glm::dot(glm::normalize(forw),glm::vec3{0,0,1}));




    glm::vec3 angles = glm::eulerAngles(q);
    glm::vec3 offset = angles;
    angles = glm::degrees(angles);
    if(ImGui::DragFloat3(name,&angles.x)){
        angles = glm::radians(angles);
        // q = angles;


        q = glm::rotate(q,angles.x - offset.x,glm::vec3(1,0,0));
        q = glm::rotate(q,angles.y - offset.y,glm::vec3(0,1,0));
        q = glm::rotate(q,angles.z - offset.z,glm::vec3(0,0,1));
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
