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


namespace YAML
{
    template <>
    struct convert<colorArray::key>
    {
        static Node encode(const colorArray::key &rhs)
        {
            Node node;
            node["id"] = rhs.id;
            node["value"] = rhs.value;
            return node;
        }

        static bool decode(const Node &node, colorArray::key &rhs)
        {
            rhs.id = node["id"].as<decltype(rhs.id)>();
            rhs.value = node["value"].as<decltype(rhs.value)>();
            return true;
        }
    };

    template <>
    struct convert<colorArray>
    {
        static Node encode(const colorArray &rhs)
        {
            Node node;
            node["keys"] = rhs.keys;
            return node;
        }

        static bool decode(const Node &node, colorArray &rhs)
        {
            rhs.keys = node["keys"].as<decltype(rhs.keys)>();
            return true;
        }
    };
}