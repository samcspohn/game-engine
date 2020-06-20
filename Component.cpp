#include "Component.h"

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
	return threadID;
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
void ComponentsUpdate(componentStorageBase *csbase, int i, int size)
{
	csbase->update(i, size);
}

void ComponentsLateUpdate(componentStorageBase *csbase, int i, int size)
{
	csbase->lateUpdate(i, size);
}

#define COMPONENT_LIST(x) static_cast<componentStorage<x> *>(allcomponents[typeid(x).hash_code()])

#define COPY(component_type)                     \
	void component_type::_copy(game_object *go)                  \
	{                                            \
		go->dupComponent(component_type(*this)); \
	}
//#define //UPDATE(component_type, update_function) void _update(int index, unsigned int _start, unsigned int _end){ \
//    listThing2<component_type>::node* i = COMPONENT_LIST(component_type)->data[_start];\
//    listThing2<component_type>::node* end;\
//    bool isEnd = _end >= COMPONENT_LIST(component_type)->data.accessor.size();\
//    if(isEnd)\
//        end = COMPONENT_LIST(component_type)->data[_end - 1];\
//    else\
//        end = COMPONENT_LIST(component_type)->data[_end];\
//	for (i; i != end; i = i->next) { i->value.threadID = index; i->value.update_function();  } \
//    if(isEnd){ end->value.threadID = index; end->value.update_function(); }\
// }
// #define //UPDATE(component_type, update_function)                                                         \
// 	void _update(int index, unsigned int _start, unsigned int _end)                                     \
// 	{                                                                                                   \
// 		deque<component_type>::iterator i = COMPONENT_LIST(component_type)->data.data.begin() + _start; \
// 		deque<component_type>::iterator end = COMPONENT_LIST(component_type)->data.data.begin() + _end; \
// 		deque<bool>::iterator val = COMPONENT_LIST(component_type)->data.valid.begin() + _start;        \
// 		for (i; i != end; ++i, ++val)                                                                   \
// 		{                                                                                               \
// 			if (*val)                                                                                   \
// 			{                                                                                           \
// 				(*i).threadID = index;                                                                  \
// 				(*i).update_function();                                                                 \
// 			}                                                                                           \
// 		}                                                                                               \
// 	}
#define LATE_UPDATE(component_type, late_update_function)                                               \
	void _lateUpdate(int index, unsigned int _start, unsigned int _end)                                 \
	{                                                                                                   \
		deque<component_type>::iterator i = COMPONENT_LIST(component_type)->data.data.begin() + _start; \
		deque<component_type>::iterator end = COMPONENT_LIST(component_type)->data.data.begin() + _end; \
		deque<bool>::iterator val = COMPONENT_LIST(component_type)->data.valid.begin() + _start;        \
		for (i; i != end; ++i, ++val)                                                                   \
		{                                                                                               \
			if (*val)                                                                                   \
			{                                                                                           \
				(*i).threadID = index;                                                                  \
				(*i).late_update_function();                                                            \
			}                                                                                           \
		}                                                                                               \
	}

//#define //UPDATE(component_type, update_function) void _update(int index, unsigned int _start, unsigned int _end){ \
//    vector<component_type>& d = COMPONENT_LIST(component_type)->data.data;\
//    for (int i = index; i < _end; i += concurrency::numThreads) { d[i].threadID = index; d[i].update_function();  }\
//}