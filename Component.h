#pragma once
#include <vector>
#include <map>
#include <list>
#include <typeinfo>
#include "Transform.h"
#include "concurrency.h"
#include "plf_list.h"
#include <stdexcept>

#define ull unsigned long long

struct compItr {
	ull hash;
	virtual void erase() {};
};

template<typename t>
struct compItr_ : public compItr {
	typename plf::list<t>::iterator it;
	plf::list<t>* l;
	void erase() {
		l->erase(it);
		delete this;
	}
	compItr_(typename plf::list<t>::iterator _it, plf::list<t>* _l) : it(_it), l(_l) {}
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
	virtual int getMin() { return 0; };
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
	vector<plf::list<t> > data;

	componentStorage() {
		data = vector<plf::list<t> >(concurrency::numThreads);
	}
	void update(int i) {
		if(data[i].size() > 0)
			((component*)& data[i].front())->_update(i);
	}
	int getMin() {
		int i = 0;
		int minimum = data[i].size();
		for (int j = 0; j < data.size(); j++) {
			if (minimum > data[j].size()) {
				minimum = data[j].size(); i = j;
			}
		}
		return i;
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
	int index = compStorage->getMin();
	compStorage->data[index].push_back(c);
	(compStorage->data[index].back()).threadID = index;

	compInfo<t> ret;
	ret.compPtr = &(compStorage->data[index].back());
	ret.CompItr = new compItr_<t>(--compStorage->data[index].end(), &compStorage->data[index]);
	ret.CompItr->hash = hash;
	componentLock.unlock();
	return ret;
}


void ComponentsUpdate(componentStorageBase* csbase, int i) {
	csbase->update(i);
}

#define COMPONENT_LIST(x) static_cast<componentStorage<x>*>(allcomponents[typeid(x).hash_code()])->data

#define COPY(component_type) void _copy(game_object* go){ \
	go->addComponent(*this); \
}
#define UPDATE(component_type, update_function) void _update(int index){ \
	typename plf::list<component_type>::iterator end = 	COMPONENT_LIST(component_type)[index].end(); \
	 for (typename plf::list<component_type>::iterator i = COMPONENT_LIST(component_type)[index].begin(); i != end; ++i) { 	i->update_function();  } \
 }
