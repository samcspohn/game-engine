#pragma once

#include <filesystem>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <string>
#include <functional>

// Define available file changes
enum class FileStatus
{
    created,
    modified,
    erased
};

class FileWatcher
{
public:
    std::string path_to_watch;
    std::vector<std::string> ignore;
    std::vector<std::string> specialize;
    bool pause = false;
    // Time interval at which we check the base folder for changes
    std::chrono::duration<int, std::milli> delay;

    // Keep a record of files from the base directory and their last modification time
    FileWatcher(std::string path_to_watch, std::chrono::duration<int, std::milli> delay) : path_to_watch{path_to_watch}, delay{delay}
    {
        
    }
    FileWatcher() = default;
    
    // Monitor "path_to_watch" for changes and in case of a change execute the user supplied "action" function
    void start(const std::function<void(std::string, FileStatus)> &action)
    {
        // for (auto &file : std::filesystem::recursive_directory_iterator(path_to_watch))
        // {
        //     paths_[file.path().string()] = std::filesystem::last_write_time(file);
        // }

        while (running_)
        {
            // Wait for "delay" milliseconds
            std::this_thread::sleep_for(delay);
            if(pause)
                continue;

            auto it = paths_.begin();
            while (it != paths_.end())
            {
                if (!std::filesystem::exists(it->first))
                {
                    action(it->first, FileStatus::erased);
                    it = paths_.erase(it);
                }
                else
                {
                    it++;
                }
            }

            // Check if a file was created or modified
            for (auto &file : std::filesystem::recursive_directory_iterator(path_to_watch))
            {
                bool brk = false;
                for(auto& ig : ignore){
                    if(file.path().string().find(ig) != -1){
                        brk = true;
                        break;
                    }
                }
                if(specialize.size() > 0 && !brk){
                    brk = true;
                    std::string f = file.path().string();
                    std::string type = f.substr(f.find_last_of("."));
                    for(auto& s : specialize){
                        if(s == type){
                            brk = false;
                            break;
                        }
                    }
                }
                if(brk)
                    continue;
                auto current_file_last_write_time = std::filesystem::last_write_time(file);
                // File creation
                if (!contains(file.path().string()))
                {
                    paths_[file.path().string()] = current_file_last_write_time;
                    action(file.path().string(), FileStatus::created);
                    // File modification
                }
                else
                {
                    if (paths_[file.path().string()] != current_file_last_write_time)
                    {
                        paths_[file.path().string()] = current_file_last_write_time;
                        action(file.path().string(), FileStatus::modified);
                    }
                }
            }
        }
    }
    void stop()
    {
        running_ = false;
    }
    YAML::Node getFileData()
    {
        YAML::Node d;
        for (auto &a : paths_)
        {
            YAML::Node file;
            file["path"] = a.first;
            file["time"] = a.second.time_since_epoch().count();
            d.push_back(file);
        }
        return d;
    }
    void getFileData(YAML::Node data)
    {
        // try
        // {
            for (int i = 0; i < data.size(); i++)
            {
                YAML::Node file = data[i];
                int64_t time = file["time"].as<int64_t>();
                std::filesystem::file_time_type t;
                t += std::chrono::duration(time * 1ns);
                paths_.emplace(file["path"].as<string>(), t);
            }
        // }
        // catch (exception e)
        // {
        // }
    }

    std::unordered_map<std::string, std::filesystem::file_time_type> paths_;
private:
    bool running_ = true;

    // Check if "paths_" contains a given key
    // If your compiler supports C++20 use paths_.contains(key) instead of this function
    bool contains(const std::string &key)
    {
        auto el = paths_.find(key);
        return el != paths_.end();
    }
};