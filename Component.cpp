#include "Component.h"
#include <omp.h>
#define ull unsigned long long

void compItr::erase(){};
component* compItr::getComponent(){};


void component::onStart() {}
void component::onDestroy() {}
bool component::_registerEngineComponent() { return false; };
void component::onCollision(game_object *go, glm::vec3 point,  glm::vec3 normal){};
// void component::_update(int index, unsigned int _start, unsigned int _end){};
void component::update(){};
void component::lateUpdate(){};
int component::getThreadID()
{
	return omp_get_thread_num();
}
ull component::getHash()
{
	return typeid(*this).hash_code();
}


std::map<ull, componentStorageBase *> allcomponents;
std::set<componentStorageBase *> gameEngineComponents;
std::set<componentStorageBase *> gameComponents;
std::mutex componentLock;

void destroyAllComponents(){
	while (allcomponents.size() > 0){
		delete allcomponents.begin()->second;
		allcomponents.erase(allcomponents.begin());
	}

}

#define COMPONENT_LIST(x) static_cast<componentStorage<x> *>(allcomponents[typeid(x).hash_code()])

#define COPY(component_type)                     \
	void component_type::_copy(game_object *go)                  \
	{                                            \
		go->dupComponent(component_type(*this)); \
	}
