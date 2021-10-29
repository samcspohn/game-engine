#include "particles/gradient.h"
#include <iostream>

_texture gradientEdit;
deque<colorArray> colorGradients;
void addColorArray(colorArray &a)
{
    colorGradients.emplace_back(a);
}

struct gradientEditorData
{
    int gradientKeyId;
    int color_picker_open;
};
unordered_map<colorArray *, gradientEditorData> gradientEditorDatas;
bool colorArrayEdit(colorArray &a, bool *p_open)
{
    gradientEditorDatas[&a].gradientKeyId = -1;
    float width = ImGui::GetWindowSize().x - 20;
    ImVec2 size = ImVec2(width, 20.0f);               // Size of the image we want to make visible
    ImVec2 uv0 = ImVec2(0.0f, 0.0f);                  // UV coordinates for lower-left
    ImVec2 uv1 = ImVec2(1.f, 1.f);                    // UV coordinates for (32,32) in our texture
    ImVec4 bg_col = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);   // Black background
    ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // No tint

    ImGuiIO &io = ImGui::GetIO();
    ImGuiStyle &style = ImGui::GetStyle();
    float radius_outer = 20.0f;
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 center = ImVec2(pos.x + radius_outer, pos.y + radius_outer);
    float line_height = ImGui::GetTextLineHeight();
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    bool changed = false;

    if (a.t.meta() == 0)
    {
        a.t._new();
        a.t.meta()->gen(100, 1);
        glm::vec4 colors[100];
        a.setColorArray(colors);
        a.t.meta()->write(&colors[0][0], GL_RGBA, GL_FLOAT);
    }

    draw_list->AddImage((void *)a.t.meta()->glid, pos, {pos.x + size.x, pos.y + size.y}, uv0, uv1, ImGui::GetColorU32(tint_col));

    // gradientEditorDatas[&a].color_picker_open = 0;
    int i = 0;
    for (auto key : a.keys)
    {

        // add carat
        float p_value = key.first * width;
        ImGui::SetCursorScreenPos({pos.x + p_value - 5, pos.y});
        bool clicked = ImGui::InvisibleButton(("pointer" + to_string(key.second.id)).c_str(), {10, 10});
        if (clicked)
        {
            gradientEditorDatas[&a].color_picker_open = i;
        }
        bool value_changed = false;
        bool is_active = ImGui::IsItemActive();
        bool is_hovered = ImGui::IsItemHovered();
        if (ImGui::IsMouseReleased)
            gradientEditorDatas[&a].gradientKeyId = -1;

        if (io.MouseDown[1] && is_hovered)
        {
            a.keys.erase(key.first);
            continue;
        }
        if (is_active && io.MouseDelta.x != 0.0f && (gradientEditorDatas[&a].gradientKeyId == -1 || key.second.id == gradientEditorDatas[&a].gradientKeyId))
        {
            p_value += io.MouseDelta.x;
            if (p_value < 0)
                p_value = 0;
            if (p_value > width - 1)
                p_value = width - 1;
            value_changed = true;
            gradientEditorDatas[&a].gradientKeyId = key.second.id;
        }
        i++;
        if (value_changed)
        {
            a.keys.erase(key.first);
            if (a.keys.find(p_value) == a.keys.end())
                a.keys.emplace(p_value / width, key.second);
            else
            {
                a.keys.emplace(p_value / width + 0.01f, key.second);
            }

            glm::vec4 colors[100];
            a.setColorArray(colors);
            a.t.meta()->write(&colors[0][0], GL_RGBA, GL_FLOAT);
            changed = true;
        }
        draw_list->AddTriangleFilled({pos.x + p_value - 5, pos.y}, {pos.x + p_value, pos.y + 10}, {pos.x + p_value + 5, pos.y}, ImGui::GetColorU32(tint_col));
        draw_list->AddTriangleFilled({pos.x + p_value - 4, pos.y}, {pos.x + p_value, pos.y + 8}, {pos.x + p_value + 4, pos.y}, ImGui::GetColorU32(*(ImVec4 *)&key.second.value));
        draw_list->AddTriangle({pos.x + p_value - 5, pos.y}, {pos.x + p_value, pos.y + 10}, {pos.x + p_value + 5, pos.y}, ImGui::GetColorU32(bg_col), 2.f);
    }
    ImGui::SetCursorScreenPos(pos);
    if (gradientEditorDatas[&a].gradientKeyId == -1)
    {
        bool clicked = ImGui::InvisibleButton("gradient_click_area", size);
        if (clicked)
        {
            cout << "clicked" << endl;
            ImVec2 m_pos = io.MousePos;
            cout << "m_pos " << m_pos.x << "," << m_pos.y << endl;
            cout << "pos " << pos.x << "," << pos.y << endl;
            float key = (float)(m_pos.x - pos.x) / (float)size.x;
            a.addKey(glm::vec4(1), key);
            gradientEditorDatas[&a].color_picker_open = 0;
            for (auto &i : a.keys)
            {
                if (i.first >= key)
                    break;
                gradientEditorDatas[&a].color_picker_open++;
            }
            // gradientKeyId = a.keys.at(key).id;
        }
    }
    ImGui::SetCursorScreenPos({pos.x, pos.y + 20 + style.ItemInnerSpacing.y});
    // if(clicked && ImGui::IsMouseClicked(2))
    //     color_picker_open = true;
    // if(color_picker_open){
    auto key = a.keys.begin();
    std::advance(key, gradientEditorDatas[&a].color_picker_open);
    if (ImGui::ColorPicker4("color_", (float *)&key->second.value))
    {
        glm::vec4 colors[100];
        a.setColorArray(colors);
        a.t.meta()->write(&colors[0][0], GL_RGBA, GL_FLOAT);
        changed = true;
        // a.keys.find(key.first)->second.value = key-second.value;
    }
    // }
    return changed;
}

int colorArray::idGenerator = 0;
colorArray &colorArray::addKey(glm::vec4 color, float position)
{
    keys.emplace(position, key(idGenerator++, color));
    return *this;
}
void colorArray::setColorArray(glm::vec4 *colors)
{
    if (keys.size() == 0)
        return;

    auto k1 = keys.begin();
    auto k2 = k1;
    ++k2;

    int end = k1->first * 100;
    for (int i = 0; i < end; ++i)
    {
        colors[i] = k1->second.value;
    }
    while (k2 != keys.end())
    {
        int start = k1->first * 100;
        int end = k2->first * 100;
        glm::vec4 step = (k2->second.value - k1->second.value) / (float)(end - start);
        int j = 0;
        for (int i = start; i < end; i++, j++)
        {
            colors[i] = k1->second.value + step * (float)(j);
        }
        k1 = k2++;
    }
    for (int i = k1->first * 100; i < 100; ++i)
    {
        colors[i] = k1->second.value;
    }
}

floatArray &floatArray::addKey(float v, float position)
{
    key k;
    k.value = v;
    k.pos = position;
    keys.push_back(k);
    return *this;
}
void floatArray::setFloatArray(float *floats)
{
    if (keys.size() == 0)
        return;
    key k = keys.front();
    if (keys.size() == 1)
    {
        for (int i = 0; i < 100; ++i)
        {
            floats[i] = k.value;
        }
    }
    else
    {
        int k1 = 0;
        int k2 = 1;
        for (int i = 0; i < keys[k1].pos * 100; ++i)
        {
            floats[i] = keys[k1].value;
        }
        while (k2 < keys.size())
        {
            float step = (keys[k2].value - keys[k1].value) / ((keys[k2].pos - keys[k1].pos) * 100);
            int start = keys[k1].pos * 100;
            int stop = keys[k2].pos * 100;
            int j = 0;
            for (int i = start; i < stop; i++, j++)
            {
                floats[i] = keys[k1].value + step * (float)j;
            }
            k1 = k2++;
        }
        for (int i = keys[k1].pos * 100; i < 100; ++i)
        {
            floats[i] = keys[k1].value;
        }
    }
}

