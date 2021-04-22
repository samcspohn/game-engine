#include "console.h"
// #include <map>


namespace console{
    std::map<std::string,int32_t> logs;
    void log(std::string s){
        if(logs.find(s) == logs.end())
            logs.emplace(s,0);
        else
            logs[s]++;
    }
}