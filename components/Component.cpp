#include "components/Component.h"

vector<int> numbers = {0};
void component::onStart() {}
void component::onDestroy() {}
void component::deinit(int id) {}
void component::init(int id) {}
bool component::_registerEngineComponent() { return false; };
void component::onCollision(collision& c){};
// void component::_update(int index, unsigned int _start, unsigned int _end){};
void component::update(){};
void component::lateUpdate(){};
int component::getThreadID()
{
	return 0;
	// return tbb::this_task_arena::current_thread_index();
}
size_t component::getHash()
{
	return typeid(*this).hash_code();
}

// std::map<ull, componentStorageBase *> componentRegistry;
// std::set<componentStorageBase *> gameEngineComponents;
// std::set<componentStorageBase *> gameComponents;
// std::mutex componentLock;

Registry ComponentRegistry;




// void save_game(const char * filename){

//     // make an archive
//     std::ofstream ofs(filename);
// 	{
//     	OARCHIVE oa(ofs);
// 		saveEmitters(oa);
// 		saveTransforms(oa);
//     	oa << ComponentRegistry;
// 	}
// 	ofs.close();
// }
// void rebuildGameObject(componentStorageBase* base, int i);

// void load_game(const char * filename)
// {
//     // open the archive
//     std::ifstream ifs(filename);
//     IARCHIVE ia(ifs);
// 	loadTransforms(ia);

//     // restore the schedule from the archive
//     ia >> ComponentRegistry;

// 	for(auto &i : ComponentRegistry.components){
// 		for(int j = 0; j < i.second->size(); j++){
// 			if(i.second->getv(j)){
// 				rebuildGameObject(i.second,j);
// 			}
// 		}
// 	}
// 	for(auto &i : ComponentRegistry.components){
// 		for(int j = 0; j < i.second->size(); j++){
// 			if(i.second->getv(j)){
// 				i.second->get(j)->onStart();
// 			}
// 		}
// 	}
// }

void destroyAllComponents(){
	// ComponentRegistry.components.clear();
	// while (ComponentRegistry.components.size() > 0){
	// 	delete ComponentRegistry.components.begin()->second;
	// 	ComponentRegistry.components.erase(ComponentRegistry.components.begin());
	// }

}

// #define COMPONENT_LIST(x) static_cast<componentStorage<x> *>(allcomponents[typeid(x).hash_code()])

