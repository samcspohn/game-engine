#include <array>
#include <chrono>
#include <imgui_widget_flamegraph.h>
#include <map>
#include <string>

#pragma once

class Profiler
{
public:

    static std::map<std::string,uint> stage_ids;
    static std::map<uint, std::string> ids_names;


    struct Scope
    {
        ImU8 _level;
        std::chrono::system_clock::time_point _start;
        std::chrono::system_clock::time_point _end;
        bool _finalized = false;
    };

    struct Entry
    {
        std::chrono::system_clock::time_point _frameStart;
        std::chrono::system_clock::time_point _frameEnd;
        std::map<uint, Scope> _stages;
    };

    void Frame()
    {
        auto& prevEntry = _entries[_currentEntry];
        _currentEntry = (_currentEntry + 1) % _bufferSize;
        prevEntry._frameEnd = _entries[_currentEntry]._frameStart = std::chrono::system_clock::now();
    }

    void Begin(const char* stage)
    {
        assert(_currentLevel < 255);
        std::map<std::string,uint>::iterator i = stage_ids.find(stage);
        if(i == stage_ids.end()){
            i = stage_ids.emplace(stage,stage_ids.size()).first;
            ids_names.emplace(i->second,i->first);
        }
        auto& entry = _entries[_currentEntry]._stages[i->second];
        entry._level = _currentLevel;
        _currentLevel++;
        entry._start = std::chrono::system_clock::now();
        entry._finalized = false;
    }

    void End(const char* stage)
    {
        assert(_currentLevel > 0);
        std::map<std::string,uint>::iterator i = stage_ids.find(stage);
        // if(i == stage_ids.end()){
        //     i = stage_ids.emplace(stage,stage_ids.size()).first;
        //     ids_names.emplace(i->second,i->first);
        // }
        auto& entry = _entries[_currentEntry]._stages[i->second];
        assert(!entry._finalized);
        _currentLevel--;
        assert(entry._level == _currentLevel);
        entry._end = std::chrono::system_clock::now();
        entry._finalized = true;
    }

    ImU8 GetCurrentEntryIndex() const
    {
        return (_currentEntry + _bufferSize - 1) % _bufferSize;
    }

    static const ImU8 _bufferSize = 100;
    std::array<Entry, _bufferSize> _entries;

private:
    ImU8 _currentEntry = _bufferSize - 1;
    ImU8 _currentLevel = 0;
};


static void ProfilerValueGetter(float* startTimestamp, float* endTimestamp, ImU8* level, const char** caption, const void* data, int idx)
{
    auto entry = reinterpret_cast<const Profiler::Entry*>(data);
    static std::map<uint,Profiler::Scope>::const_iterator __i;
    // auto& stage = entry->_stages[idx];
    if(idx == 0){
        __i = entry->_stages.begin();
    }else{
        ++__i;
    }
    auto& stage = __i->second;
    if (startTimestamp)
    {
        std::chrono::duration<float, std::milli> fltStart = stage._start - entry->_frameStart;
        *startTimestamp = fltStart.count();
    }
    if (endTimestamp)
    {
        *endTimestamp = stage._end.time_since_epoch().count() / 1000000.0f;

        std::chrono::duration<float, std::milli> fltEnd = stage._end - entry->_frameStart;
        *endTimestamp = fltEnd.count();
    }
    if (level)
    {
        *level = stage._level;
    }
    if (caption)
    {
        *caption = Profiler::ids_names[__i->first].c_str();
    }
}
