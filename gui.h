#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <vector>
#include <list>
#include <string>

namespace gui{

class gui_base{
public:
    virtual inline void render(){}
};

std::list<gui_base*> gui_windows;

class window : public gui_base{
public:
    std::vector<gui_base*> children;
    ImVec2 pos;
    ImVec2 size;
    ImGuiWindowFlags flags = 0;
    std::string name;
    bool p_open = true;
    typename std::list<gui_base*>::iterator itr;
    window(){
        gui_windows.push_back(this);
        itr = --gui_windows.end();
    }
    inline void render(){
        ImGui::SetNextWindowPos(pos, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);

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
}