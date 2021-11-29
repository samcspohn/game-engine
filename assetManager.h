#include "editor.h"
#include <unordered_map>
#include <string>
#include <vector>
using namespace std;

struct assetManager{
    unordered_map<string,assets::assetManagerBase*> assetManagers;
    void registerAssetManager(vector<string> extensions, assets::assetManagerBase* am){
        for(string& s : extensions){
            assetManagers.emplace(s,am);
        }
    }
    void load(string path){
        string ext = path.substr(path.find_last_of("."));
        auto i = assetManagers.find(ext);
        if(i != assetManagers.end()){
            i->second->load(path);
        }
    }
};