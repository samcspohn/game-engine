#pragma once
#include <glm/glm.hpp>
#include <map>
#include <vector>
#include <array>
#include "texture.h"
#include <set>
#include "_serialize.h"

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
    std::map<_texture, rect> uvMap;
    // std::map<int, std::map<int, rect>> coords;
    // std::vector<rect> avail;

    texAtlas(string name);

    bool addTexture(_texture _t);
};
