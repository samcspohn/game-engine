#pragma once
#include "Transform.h"
#include <map>
#include "Component.h"
#include <set>
#include "plf_list.h"
#include "fast_list.h"
using namespace std;

mutex removeLock;
std::set<compItr*> toRemove;
mutex destroyLock;
std::set<Transform*> toDestroy;

class _renderer;
class game_object {

	map<ull, compItr*> components;
	~game_object() { };
	//friend component;
	typename fast_list_deque<_renderer>::iterator renderer = 0;
	friend _renderer;
public:

	_renderer* getRenderer() {
		return (_renderer*)&*renderer;
	}
	template<typename t>
    typename fast_list_deque<t>::iterator getComponent() {
		ull hash = typeid(t).hash_code();
		for (auto& i : components)
			if (i.second->hash == hash){
                return dynamic_cast<compItr_<t>*>(i.second)->get();
//				return ((compItr_<t>*)i.second)->get();
			}
        throw;
//		return 0;
	}


	template<class t>
	typename fast_list_deque<t>::iterator addComponent() {
		compInfo<t> ci = addComponentToAll(t());
		typename fast_list_deque<t>::iterator ret = ci.compPtr;
		components[typeid(t).hash_code()] = ci.CompItr;
		((component*)&(*ret))->transform = this->transform;
		((component*)&(*ret))->transform->gameObject = this;
		((component*)&(*ret))->onStart();
		return ret;
	}

	template<class t>
	typename fast_list_deque<t>::iterator addComponent(const t& c) {
		compInfo<t> ci = addComponentToAll(c);
		typename fast_list_deque<t>::iterator ret = ci.compPtr;
		components[typeid(t).hash_code()] = ci.CompItr;
		((component*)&(*ret))->transform = this->transform;
		((component*)&(*ret))->transform->gameObject = this;
		((component*)&(*ret))->onStart();
		return ret;
	}

	template<class t>
	void removeComponent(typename fast_list_deque<t>::iterator c) {
		removeLock.lock();
		if(toRemove.find(components.at(typeid(t).hash_code())) != toRemove.end()){
            cout << "already removed" << endl;
            throw;
		}
		toRemove.insert(components.at(typeid(t).hash_code()));
		removeLock.unlock();
		components.erase(typeid(t).hash_code());
	}
    void removeComponent(ull h) {
		removeLock.lock();
		if(toRemove.find(components.at(h)) != toRemove.end()){
            cout << "already removed" << endl;
            throw;
		}
		toRemove.insert(components.at(h));
		removeLock.unlock();
		components.erase(h);
	}
    template<class t>
	void removeComponent() {
		removeLock.lock();
		typename fast_list_deque<t>::iterator c = getComponent<t>();
		if(toRemove.find(components.at(typeid(t).hash_code())) != toRemove.end()){
            cout << "already removed" << endl;
            throw;
		}
		toRemove.insert(components.at(typeid(t).hash_code()));
		removeLock.unlock();
		components.erase(typeid(t).hash_code());
	}

	void destroy() {
		destroyLock.lock();
		while (components.size() > 0) {
			removeComponent(components.begin()->first);
		}
		if(toDestroy.find(transform) != toDestroy.end()){
            cout << "already destroyed" << endl;
            throw;
		}
		toDestroy.insert(transform);
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
			i.second->getComponent()->_copy(this);
		}
	}

	void _destroy() {

		delete this;
	}

	Transform* transform;

};

game_object* rootGameObject;
