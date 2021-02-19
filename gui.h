#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <vector>
#include <list>
#include <string>
#include "texture.h"
#include <functional>

namespace gui{

class gui_base{
public:
    virtual void render();
};

extern std::list<gui_base*> gui_windows;

class window : public gui_base{
public:
    std::vector<gui_base*> children;
    ImVec2 pos = ImVec2(20,20);
    ImVec2 size = ImVec2(60,60);
    ImGuiWindowFlags flags = ImGuiWindowFlags_None;
    std::string name;
    bool p_open = true;
    typename std::list<gui_base*>::iterator itr;
    enum ImGuiCond_ loadConfig = ImGuiCond_Once;
    window();
    window(const char* n);
    void adopt(gui_base* g);
    void render();
    ~window();
};


class text : public gui_base{
public:
    std::string contents;
    void render();
};

class image : public gui_base{
public:
    ImVec2 pos;
    _texture img;
    void render();
};

class button : public gui_base{
public:
    std::string label;
    ImVec2 size{-FLT_MIN, 0.0f};
    std::function<void()> callBack;
    void render();
};


class tree : public gui_base{
public:
    std::string label;
    // ImVec2 size;
    vector<gui_base*> children;
    bool selected{false};
    void render();
};


}
