#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <iostream>
#include "fileWatcher.h"
#include "components/Component.h"
#include <fstream>
#include <unordered_map>
#include <thread>
#include <set>
#include <mutex>
#include <filesystem>
namespace fs = std::filesystem;


#ifdef RELEASE
const char* opt_level = "-O3 -ffast-math "; 
#else 
const char* opt_level = "";
#endif

struct runtimeCompiler
{
    FileWatcher fw;
    std::thread *t;
    std::unordered_map<std::string, void *> file_map;
    std::mutex lock;
    bool compiling = false;
    bool compilationComplete = false;
    bool compilationSuccess = false;
    vector<std::string> include;
    std::string getCppName(std::string path)
    {
        return path.substr(path.find_last_of("/") + 1);
    }
    std::string getRuntimeSO(std::string _path){
        return fw.path_to_watch + "/runtime/" + getCppName(_path) + ".so";
    }
    void compileCpp(std::string &path_to_watch)
    {
        std::string path = fw.path_to_watch + "/runtime/" + getCppName(path_to_watch);
        compiling = true;
        string includes = "-I" + fw.path_to_watch + "/..";
        for (auto &i : include)
        {
            includes += " -I" + fw.path_to_watch + "/../" + i;
        }
        std::cout << includes << std::endl;
        string compile = "g++ -DNDEBUG -fPIC -fpermissive " + string(opt_level) + includes + " -g -c -o " + path + ".o " + path_to_watch;
        std::cout << compile << std::endl;
        int cppSuccess = system(compile.c_str());
        if (cppSuccess == 0)
        { // create shared if compilation succeeds
            system(("gcc -shared -o " + path + ".so " + path + ".o").c_str());

            std::string so = path + ".so";
            if (file_map.find(so) == file_map.end())
            {
                file_map.emplace(so, (void *)0);
            }
            compilationComplete = true;
            compilationSuccess = true;
        }
        else
        {
            compilationComplete = true;
            compilationSuccess = false;
        }
        compiling = false;
    }
    void reloadModules()
    {
        lock.lock();
        for (auto &i : file_map)
        {
            if (file_map.at(i.first) != 0)
            {
                std::cout << "unloading: " << i.first << std::endl;
                dlclose(file_map.at(i.first));
            }
            void *handle;
            handle = dlopen(i.first.c_str(), RTLD_LAZY);
            if (!handle)
            {
                fprintf(stderr, "%s\n", dlerror());
                // exit(1);
                std::cout << "failed to load" << i.first << std::endl;
            }
            file_map.at(i.first) = handle;
        }
        lock.unlock();
    }
    runtimeCompiler()
    {
    }
    bool getCompiling()
    {
        return compiling;
    }
    bool getCompilationComplete()
    {
        if (compilationComplete)
        {
            compilationComplete = false;
            return true;
        }
        return false;
    }
    bool getCompilationSuccess()
    {
        if (compilationSuccess)
        {
            compilationSuccess = false;
            return true;
        }
        return false;
    }
    void initLoadedScripts()
    {
        std::set<std::string> cpp_files;

        for (auto &i : fw.paths_)
        {
            if (i.first.find(".cpp") != -1  && i.first.find(".o") == -1 && i.first.find(".so") == -1)
            {
                cpp_files.emplace(i.first);
                if(!fs::exists(getRuntimeSO(i.first))){
                    string s = i.first;
                    compileCpp(s);
                }
            }
            if (i.first.find(".so") != -1 && fs::exists(i.first))
            {
                file_map.emplace(i.first, (void *)0);
            }
        }
        for (auto &i : cpp_files)
        {
            std::string so = fw.path_to_watch + "/runtime/" + getCppName(i) + ".so";
            if (file_map.find(so) == file_map.end())
            {
                std::filesystem::file_time_type t;
                fw.paths_.at(so) = t;
                // compile cpp
            }
        }
        reloadModules();
        // for(auto& i : fw.paths_){
        //     if(i.first.find(".so")){
        //         cpp_files.emplace(i.first);
        //     }
        // }
    }
    void run(std::string s)
    {
        // fw = FileWatcher{s, std::chrono::milliseconds(500)};
        system(("mkdir " + s + "/runtime").c_str());
        fw.delay = std::chrono::milliseconds(500);
        fw.path_to_watch = s;
        // fw.ignore = {"runtime"};
        fw.specialize = {".cpp", ".so", ".o"};
        initLoadedScripts();
        t = new std::thread([&]()
                            { fw.start([&](std::string path_to_watch, FileStatus status) -> void
                                       {
                                             // Process only regular files, all other file types are ignored
                                             if (!std::filesystem::is_regular_file(std::filesystem::path(path_to_watch)) && status != FileStatus::erased)
                                             {
                                                 return;
                                             }
                                             auto handleFile = [&]()
                                             {
                                                 if (path_to_watch.find(".cpp") != -1 && path_to_watch.find(".o") == -1 && path_to_watch.find(".so") == -1)
                                                     compileCpp(path_to_watch);
                                             };
                                             auto handleFileErased = [&]()
                                             {
                                                 int i;
                                                 if (i = path_to_watch.find(".cpp") != -1 && path_to_watch.find(".o") == -1 && path_to_watch.find(".so") == -1)
                                                 {
                                                    std::string path = path_to_watch.substr(0, path_to_watch.find(".cpp"));
                                                    path = fw.path_to_watch + "/runtime/" + getCppName(path_to_watch);
                                                     std::string o = path + ".o";
                                                     std::string so = path + ".so";
                                                     system(("rm " + o).c_str());
                                                     system(("rm " + so).c_str());
                                                     dlclose(file_map.at(so));
                                                     file_map.erase(so);
                                                 }
                                             };

                                             switch (status)
                                             {
                                             case FileStatus::created:
                                                 std::cout << "File created: " << path_to_watch << '\n';
                                                 handleFile();
                                                 break;
                                             case FileStatus::modified:
                                                 std::cout << "File modified: " << path_to_watch << '\n';
                                                 handleFile();
                                                 break;
                                             case FileStatus::erased:
                                                 std::cout << "File erased: " << path_to_watch << '\n';
                                                 handleFileErased();
                                                 break;
                                             default:
                                                 std::cout << "Error! Unknown file status.\n";
                                             }; }); });
    }
    void stop()
    {
        fw.stop();
        t->join();
        delete t;
    }
};
