#include "gui.h"
namespace gui
{

    void gui_base::render() {}

    std::list<gui_base *> gui_windows;

    window::window()
    {
        gui_windows.push_back(this);
        itr = --gui_windows.end();
    }
    window::window(const char *n) : name(n)
    {
        gui_windows.push_back(this);
        itr = --gui_windows.end();
    }
    void window::adopt(gui_base *g)
    {
        children.push_back(g);
    }
    void window::render()
    {
        ImGui::SetNextWindowPos(pos, loadConfig);
        ImGui::SetNextWindowSize(size, loadConfig);

        // // Main body of the Demo window starts here.
        // flags |= ImGuiWindowFlags_NoTitleBar;
        // flags |= ImGuiWindowFlags_NoMove;
        // flags |= ImGuiWindowFlags_NoResize;
        // flags |= ImGuiWindowFlags_NavFlattened;
        ImGui::Begin(name.c_str(), &p_open, flags);
        for (auto &i : children)
        {
            i->render();
        }
        ImGui::End();
    }
    window::~window()
    {
        gui_windows.erase(itr);
    }

    void text::render()
    {
        ImGui::Text(contents.c_str());
    }

    void image::render()
    {
        ImVec2 currPos = ImGui::GetCursorPos();
        ImGui::SetCursorPos(pos);
        ImGui::Image(ImTextureID(img.t->id), ImVec2(img.t->dims.x, img.t->dims.y), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 0));
        ImGui::SetCursorPos(currPos);
    }
    void button::render()
    {
        if (ImGui::Button(label.c_str(), size))
            callBack();
    }

    void tree::render()
    {
        if (ImGui::TreeNode(label.c_str()))
        {
            for (auto &i : children)
                i->render();
            ImGui::TreePop();
        }
    }

}
