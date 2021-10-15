#include <glm/glm.hpp>
#include <map>
#include <vector>
#include <array>
#include "texture.h"

struct rect
{
    glm::vec2 coord;
    glm::vec2 sz;
};

struct texAtlas
{
    glm::ivec2 sz;
    _texture atlas;
    std::set<_texture> textures;
    std::map<_texture,rect> uvMap;
    // std::map<int, std::map<int, rect>> coords;
    // std::vector<rect> avail;

    texAtlas(string name){
        atlas.namedTexture(name);
    }

    bool addTexture(_texture _t)
    {
        // if(x > sz.x || y > sz.y){
        //resize texture;
        // }
        sz.x = std::max(sz.x, _t.t->dims.x);
        sz.y = sz.y + _t.t->dims.y;
        vector<glm::u8vec4> pixels(sz.x * sz.y);
        atlas.t->gen(sz.x, sz.y, GL_RGBA);
        int tex_size = textures.size();
        textures.emplace(_t);

        int y_axis = 0;
        auto xy = [&](int a, int b)
        {
            return a + b * sz.y;
        };
        for (auto &t : textures)
        {
            vector<glm::u8vec4> colors;
            colors.resize(t.t->dims.x * t.t->dims.y);
            t.t->read(colors.data());
            uvMap[t].coord = {0.f,float(y_axis) / float(sz.y)};
            uvMap[t].sz = {float(t.t->dims.x) / float(sz.x),float(t.t->dims.y) / float(sz.y)};

            auto _xy = [&](int a, int b)
            {
                return a + b * t.t->dims.y;
            };
            for (int i = 0; i < t.t->dims.x; i++)
            {
                for (int j = 0; j < t.t->dims.y; j++)
                {
                    pixels[xy(i,y_axis)] = colors[_xy(i,j)];
                }
                y_axis++;
            }
        }
        atlas.t->write(pixels.data());
        return tex_size != textures.size();
    }
};