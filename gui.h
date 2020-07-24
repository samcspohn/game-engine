#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <vector>
#include <list>
#include <string>
#include "texture.h"

namespace gui{

class gui_base{
public:
    virtual inline void render(){}
};

std::list<gui_base*> gui_windows;

class window : public gui_base{
public:
    std::vector<gui_base*> children;
    ImVec2 pos = ImVec2(20,20);
    ImVec2 size = ImVec2(60,60);
    ImGuiWindowFlags flags = 0;
    std::string name;
    bool p_open = true;
    typename std::list<gui_base*>::iterator itr;
    window(){
        gui_windows.push_back(this);
        itr = --gui_windows.end();
    }
    void adopt(gui_base* g){
        children.push_back(g);
    }
    inline void render(){
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(size, ImGuiCond_Always);

        // // Main body of the Demo window starts here.
        // flags |= ImGuiWindowFlags_NoTitleBar;
        // flags |= ImGuiWindowFlags_NoMove;
        // flags |= ImGuiWindowFlags_NoResize;
        // flags |= ImGuiWindowFlags_NoBackground;
        ImGui::Begin(name.c_str(), &p_open, flags);  
         for(auto& i : children){
            i->render();
        }
        ImGui::End();
    }
    ~window(){
        gui_windows.erase(itr);
    }
};


class text : public gui_base{
public:
    std::string contents;
    inline  void render(){
        ImGui::Text(contents.c_str());
    }
};

class image : public gui_base{
public:
    ImVec2 pos;
    Texture img;
    inline  void render(){
        ImVec2 currPos = ImGui::GetCursorPos();
        ImGui::SetCursorPos(pos);
        ImGui::Image(ImTextureID(img.id), ImVec2(img.dims.x, img.dims.y), ImVec2(0,0), ImVec2(1,1), ImColor(255,255,255,255), ImColor(255,255,255,0));
        ImGui::SetCursorPos(currPos);
    }
};

}
