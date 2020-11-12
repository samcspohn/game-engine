#include "Component.h"
#define ull unsigned long long

void compItr::erase(){};
component* compItr::getComponent(){ return 0; };

vector<int> numbers = {0};
void component::onStart() {}
void component::onDestroy() {}
bool component::_registerEngineComponent() { return false; };
void component::onCollision(game_object *go, glm::vec3 point,  glm::vec3 normal){};
// void component::_update(int index, unsigned int _start, unsigned int _end){};
void component::update(){};
void component::lateUpdate(){};
int component::getThreadID()
{
	return tbb::this_task_arena::current_thread_index();
}
ull component::getHash()
{
	return typeid(*this).hash_code();
}

tbb::affinity_partitioner update_ap;

// std::map<ull, componentStorageBase *> componentRegistry;
// std::set<componentStorageBase *> gameEngineComponents;
// std::set<componentStorageBase *> gameComponents;
// std::mutex componentLock;

Registry ComponentRegistry;

void save_game(const char * filename){

    // make an archive
    std::ofstream ofs(filename);
	{
    	boost::archive::text_oarchive oa(ofs);
		saveTransforms(oa);
    	oa << ComponentRegistry;
	}
	ofs.close();
}
void rebuildGameObject(componentStorageBase* base, int i);

void load_game(const char * filename)
{
    // open the archive
    std::ifstream ifs(filename);
    boost::archive::text_iarchive ia(ifs);
	loadTransforms(ia);

    // restore the schedule from the archive
    ia >> ComponentRegistry;

	for(auto &i : ComponentRegistry.components){
		for(int j = 0; j < i.second->size(); j++){
			if(i.second->getv(j)){
				rebuildGameObject(i.second,j);
			}
		}
	}
	for(auto &i : ComponentRegistry.components){
		for(int j = 0; j < i.second->size(); j++){
			if(i.second->getv(j)){
				i.second->get(j)->onStart();
			}
		}
	}
}

void destroyAllComponents(){
	while (ComponentRegistry.components.size() > 0){
		delete ComponentRegistry.components.begin()->second;
		ComponentRegistry.components.erase(ComponentRegistry.components.begin());
	}

}

// #define COMPONENT_LIST(x) static_cast<componentStorage<x> *>(allcomponents[typeid(x).hash_code()])

