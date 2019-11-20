#pragma once
#include <vector>
#include <map>
#include <list>
#include <typeinfo>
#include "Transform.h"
#include "concurrency.h"
#include "plf_list.h"
#include <stdexcept>
#include "listThing2.h"
#include "fast_list.h"

#define ull unsigned long long

struct compItr {
	ull hash;
	virtual void erase() {};
};

template<typename t>
struct compItr_ : public compItr {
    typename fast_list_deque<t*>::iterator id;
	fast_list_deque<t*>* l;
	void erase() {
	    delete *id;
		l->erase(id);
		delete this;
	}
	compItr_(typename fast_list_deque<t*>::iterator _id, fast_list_deque<t*>* _l) : id(_id), l(_l) {}
	compItr_() {}
};

template<typename t>
struct compInfo {
	t* compPtr;
	compItr* CompItr;
};

class componentStorageBase {
public:
	string name;

	virtual void update(int index) {}
	virtual void reset(){};
};

class game_object;
class Transform;
class component {
public:
	int threadID;
	virtual void onStart() {}


	virtual void _update(int index, unsigned int _start, unsigned int _end) {};
	virtual void _copy(game_object* go) = 0;
	Transform* transform;
	int getThreadID() {
		return threadID;
	}
	ull getHash() {
		return typeid(*this).hash_code();
	}
};


template<typename t>
class componentStorage : public componentStorageBase {
public:
	fast_list_deque<t*> data;

	componentStorage() {
//		data = listThing2<t>();
	}
	void reset(){}
	void update(int index) {
//	    if(data.accessor.size() / concurrency::numThreads * (index+1) - data.accessor.size() / concurrency::numThreads * index > 0){
            unsigned int _start = (unsigned int)(data.data.size() / concurrency::numThreads * index);
            unsigned int _end = (unsigned int)(data.data.size() / concurrency::numThreads * (index + 1));
            if(index == concurrency::numThreads - 1)
                _end = data.data.size();
            ((component*)data.data.front())->_update(index, _start, _end);
//	    }
	}
};

std::map<ull, componentStorageBase*> allcomponents;

std::mutex componentLock;
template<typename t>
inline compInfo<t> addComponentToAll(t* c) {

	componentLock.lock();
	ull hash = typeid(t).hash_code();

	if (allcomponents.find(hash) == allcomponents.end()) {
		allcomponents[hash] = (componentStorageBase*)(new componentStorage<t>());
		allcomponents[hash]->name = typeid(t).name();
	}
	componentStorage<t>* compStorage = static_cast<componentStorage<t>*>(allcomponents[hash]);
	typename fast_list_deque<t*>::iterator id = compStorage->data.push_back(c);

	compInfo<t> ret;
	ret.compPtr = (compStorage->data.data[id]);
	ret.CompItr = new compItr_<t>(id, &compStorage->data);
	ret.CompItr->hash = hash;
	componentLock.unlock();
	return ret;
}


void ComponentsUpdate(componentStorageBase* csbase, int i) {
	csbase->update(i);
}

#define COMPONENT_LIST(x) static_cast<componentStorage<x>*>(allcomponents[typeid(x).hash_code()])

#define COPY(component_type) void _copy(game_object* go){ \
	go->addComponent(new component_type(*this)); \
}
//#define UPDATE(component_type, update_function) void _update(int index, unsigned int _start, unsigned int _end){ \
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
#define UPDATE(component_type, update_function) void _update(int index, unsigned int _start, unsigned int _end){ \
    deque<component_type*>::iterator i = COMPONENT_LIST(component_type)->data.data.begin() + _start;\
    deque<component_type*>::iterator end = COMPONENT_LIST(component_type)->data.data.begin() + _end;\
    for (i; i != end; ++i) { (*i)->threadID = index; (*i)->update_function();  }\
}
