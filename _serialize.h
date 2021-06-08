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

#define SER_FUNC()                                            \
    void ser_edit(ser_mode x, YAML::Node &node_9738469372465) \
    {

#define SER_END                                                                  \
    }                                                                            \
                                                                                 \
    friend class boost::serialization::access;                                   \
                                                                                 \
    inline void serialize(IARCHIVE &ar, const unsigned int /* file_version */)   \
    {                                                                            \
        YAML::Node none;                                                         \
        ar >> boost::serialization::base_object<component>(*this);               \
        _iar = &ar;                                                              \
        ser_edit(ser_mode::read_mode, none);                                     \
    }                                                                            \
    inline void serialize(OARCHIVE &ar, const unsigned int /* file_version */)   \
    {                                                                            \
                                                                                 \
        YAML::Node none;                                                         \
        ar << boost::serialization::base_object<component>(*this);               \
        _oar = &ar;                                                              \
        ser_edit(ser_mode::write_mode, none);                                    \
    }                                                                            \
    template <class Archive>                                                     \
    inline void serialize(Archive &ar, const unsigned int /* file_version */) {} \
    inline void encode(YAML::Node &node)                                         \
    {                                                                            \
        ser_edit(ser_mode::write_mode, node);                                    \
    }                                                                            \
    inline void decode(YAML::Node &node)                                         \
    {                                                                            \
        ser_edit(ser_mode::read_mode, node);                                     \
    }

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