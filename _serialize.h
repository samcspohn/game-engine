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

#define SER_FUNC()            \
    void ser_edit(ser_mode x) \
    {

#define SER_END                                                                \
    }                                                                          \
                                                                               \
    friend class boost::serialization::access;                                 \
                                                                               \
    inline void serialize(IARCHIVE &ar, const unsigned int /* file_version */) \
    {                                                                          \
        ar >> boost::serialization::base_object<component>(*this);             \
        _iar = &ar;                                                            \
        ser_edit(ser_mode::read_mode);                                         \
    }                                                                          \
    inline void serialize(OARCHIVE &ar, const unsigned int /* file_version */) \
    {                                                                          \
        ar << boost::serialization::base_object<component>(*this);             \
        _oar = &ar;                                                            \
        ser_edit(ser_mode::write_mode);                                        \
    }                                                                          \
    template <class Archive>                                                   \
    inline void serialize(Archive &ar, const unsigned int /* file_version */) {}

#define SER(var)                    \
    switch (x)                      \
    {                               \
    case ser_mode::edit_mode:       \
        renderEdit(#var, var);      \
        break;                      \
    case ser_mode::read_mode:       \
        (*_iar) >> var;             \
        break;                      \
    case ser_mode::write_mode:      \
        (*_oar) << var;             \
        break;                      \
    default:                        \
        cout << "no mode provided"; \
        break;                      \
    }