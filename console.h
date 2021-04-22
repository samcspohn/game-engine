#pragma once
#include <string>
#include <map>

namespace console{
    extern std::map<std::string,int32_t> logs;
    void log(std::string s);
}