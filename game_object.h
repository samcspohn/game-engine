#pragma once
#include "Transform.h"
#include <map>
#include "Component.h"
#include <set>
#include "plf_list.h"
using namespace std;

mutex removeLock;
plf::list<compItr*> toRemove;
mutex destroyLock;
std::list<Transform*> toDestroy;

class _renderer;
class game_object {

	map<component*, compItr*> components;
	~game_object() { };
	//friend component;
	component* renderer = 0;
	friend _renderer;
public:

	_renderer* getRenderer() {
		return (_renderer*)renderer;
	}
	template<typename t>
	t* getComponent() {
		ull hash = typeid(t).hash_code();
		for (auto& i : components)
			if (i.second->hash == hash)
				return (t*)i.first;
		return 0;
	}


	template<class t>
	t * addComponent() {
		compInfo<t> ci = addComponentToAll(new t());
		t* ret = ci.compPtr;
		components[ret] = ci.CompItr;
		((component*)ret)->transform = this->transform;
		((component*)ret)->transform->gameObject = this;
		ci.compPtr->onStart();
		return ret;
	}

	template<class t>
	t* addComponent(t* c) {
		compInfo<t> ci = addComponentToAll(c);
		t* ret = ci.compPtr;
		components[ret] = ci.CompItr;
		((component*)ret)->transform = this->transform;
		((component*)ret)->transform->gameObject = this;
		ci.compPtr->onStart();
		return ret;
	}

	template<class t>
	void removeComponent(t* c) {
		removeLock.lock();
		toRemove.push_back(components.at(c));
		removeLock.unlock();
		components.erase(c);
	}
    template<class t>
	void removeComponent() {
		removeLock.lock();
		component* c = getComponent<t>();
		toRemove.push_back(components.at(c));
		removeLock.unlock();
		components.erase(c);
	}

	void destroy() {
		destroyLock.lock();
		while (components.size() > 0) {
			removeComponent(components.begin()->first);
		}
		toDestroy.push_back(transform);
		destroyLock.unlock();
		for(auto& i : transform->getChildren()){
            i->gameObject->destroy();
		}
	}

	game_object(Transform* t) {
		this->transform = t;
		t->gameObject = this;
	}
	game_object() {
		this->transform = new Transform(this);
		root->Adopt(this->transform);
	};
	game_object(const game_object & g) {
		this->transform = new Transform(*g.transform, this);
		g.transform->getParent()->Adopt(this->transform);
		for (auto& i : g.components) {
			i.first->_copy(this);
		}
	}

	void _destroy() {

		delete this;
	}

	Transform* transform;

};

game_object* rootGameObject;
