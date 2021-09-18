#pragma once
#include <fstream>
#include "serialize.h"
#include "editor.h"

extern IARCHIVE *_iar;
extern OARCHIVE *_oar;
enum ser_mode
{
    edit_mode,
    read_mode,
    write_mode
};

#define SER_FUNC()                            \
    inline void encode(YAML::Node &node)      \
    {                                         \
        ser_edit(ser_mode::write_mode, node); \
    }                                         \
    inline void decode(YAML::Node &node)      \
    {                                         \
        ser_edit(ser_mode::read_mode, node);  \
    }                                         \
    void ser_edit(ser_mode x, YAML::Node &node_9738469372465)

// void decode(YAML::Node &node, const char *name, float &value)
// {
//     value = node[name].as<float>();
// }

#define SER(var)                                            \
    switch (x)                                              \
    {                                                       \
    case ser_mode::edit_mode:                               \
        renderEdit(#var, var);                              \
        break;                                              \
    case ser_mode::read_mode:                               \
        /*(*_iar) >> var;*/                                 \
        var = node_9738469372465[#var].as<decltype(var)>(); \
        break;                                              \
    case ser_mode::write_mode:                              \
        /*(*_oar) << var;*/                                 \
        node_9738469372465[#var] = var;                     \
        break;                                              \
    default:                                                \
        cout << "no mode provided";                         \
        break;                                              \
    }