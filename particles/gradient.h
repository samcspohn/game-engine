#pragma once

#include <map>
#include <unordered_map>
#include "_serialize.h"
#include "_rendering/texture.h"
#include <glm/glm.hpp>

struct colorArray
{
    struct key{
        int id;
        glm::vec4 value;
        SER_HELPER(){
            ar & id & value;
        }
        key(int _id, glm::vec4 _value) : id(_id), value(_value) {

        }
        key() = default;
    };
    static int idGenerator;
    _texture t;
    map<float,key> keys = {{0.f,key(idGenerator++,glm::vec4(1))}};
    colorArray &addKey(glm::vec4 color, float position);
    void setColorArray(glm::vec4 *colors);
    SER_HELPER(){
        ar & keys;
    }
};

extern std::deque<colorArray> colorGradients;
extern int gradient_index;
void addColorArray(colorArray& c);
extern _texture gradientEdit;
bool colorArrayEdit(colorArray &a, bool *p_open);

struct floatArray
{
    struct key
    {
        float value;
        float pos;
    };
    std::vector<key> keys;
    floatArray &addKey(float v, float position);
    void setFloatArray(float *floats);
};