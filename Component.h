#pragma once
#include <vector>
#include <map>
#include <list>
#include <typeinfo>
#include "Transform.h"
#include "concurrency.h"
#include "plf_list.h"
#include <stdexcept>
#include "listThing.h"

#define ull unsigned long long

struct compItr {
	ull hash;
	virtual void erase() {};
};

template<typename t>
struct compItr_ : public compItr {
    typename listThing<t>::elemRef* itp;
	listThing<t>* l;
	void erase() {
		l->erase(itp->it);
		delete this;
	}
	compItr_(typename listThing<t>::elemRef* _itp, listThing<t>* _l) : itp(_itp), l(_l) {}
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

	virtual void update(int i) {}
	virtual void rebalance() {};
};

class game_object;
class Transform;
class component {
public:
	int threadID;
	virtual void onStart() {}


	virtual void _update(int index) {};
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
	listThing<t> data;

	componentStorage() {
//		data = listThing<t>();
	}
	void update(int index) {
		if(data.bookmarks[index].count > 0)
            ((component*)& data.data.front())->_update(index);
	}
	void rebalance(){
        data.rebalance();
	}
};

std::map<ull, componentStorageBase*> allcomponents;

std::mutex componentLock;
template<typename t>
inline compInfo<t> addComponentToAll(const t& c) {

	componentLock.lock();
	ull hash = typeid(c).hash_code();

	if (allcomponents.find(hash) == allcomponents.end()) {
		allcomponents[hash] = (componentStorageBase*)(new componentStorage<t>());
		allcomponents[hash]->name = typeid(t).name();
	}
	componentStorage<t>* compStorage = static_cast<componentStorage<t>*>(allcomponents[hash]);
	auto it = compStorage->data.insert(c);

	compInfo<t> ret;
	ret.compPtr = &(compStorage->data.data.back());
	ret.CompItr = new compItr_<t>(it->ref, &compStorage->data);
	ret.CompItr->hash = hash;
	componentLock.unlock();
	return ret;
}


void ComponentsUpdate(componentStorageBase* csbase, int i) {
	csbase->update(i);
}

#define COMPONENT_LIST(x) static_cast<componentStorage<x>*>(allcomponents[typeid(x).hash_code()])

#define COPY(component_type) void _copy(game_object* go){ \
	go->addComponent(*this); \
}
#define UPDATE(component_type, update_function) void _update(int index){ \
    LIST_THING_STORAGE<component_type>::iterator i = COMPONENT_LIST(component_type)->data.bookmarks[index].it->it;\
    const LIST_THING_STORAGE<component_type>::iterator end = (index < concurrency::numThreads-1 ? COMPONENT_LIST(component_type)->data.bookmarks[index+1].it->it : COMPONENT_LIST(component_type)->data.data.end());\
	for (i; i != end; ++i) { i->threadID = index; i->update_function();  } \
 }
