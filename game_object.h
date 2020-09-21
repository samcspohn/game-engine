#pragma once
#include "Component.h"
#include "Transform.h"
#include <map>
#include <set>
#include <unordered_set>
#include "plf_list.h"
#include "fast_list.h"

using namespace std;

class _renderer;
class game_object_proto{
public:
	map<component *, ull> components;

	game_object_proto(){}
	// game_object_proto(const game_object_proto& other){
	// 	for()
	// }
	template <class t>
	t *addComponent()
	{
		component* ret = new t();
		this->components.insert(std::pair<component *, ull>(ret,typeid(t).hash_code()));
		return (t*)ret;
	}

	template <typename t>
	t *getComponent()
	{
		ull hash = typeid(t).hash_code();
		for (auto &i : components)
			if (i.second == hash)
			{
				return (t *)i.first;
			}
		return 0;
	}
	
};

class game_object;

extern tbb::concurrent_unordered_set<game_object*> toDestroy;

class game_object
{
	mutex lock;
	mutex colLock;
	bool colliding = false;

	map<component *, compItr *> components;
	~game_object() { destroyed = false; };
	bool destroyed = false;
	//friend component;
	_renderer *renderer = 0;
	friend _renderer;

public:
	_renderer *getRenderer()
	{
		return (_renderer *)&*renderer;
	}
	template <typename t>
	t *getComponent()
	{
		ull hash = typeid(t).hash_code();
		for (auto &i : components)
			if (i.second->hash == hash)
			{
				return (t *)i.first;
			}
		return 0;
	}
	template <typename t>
	vector<t*> getComponents(){
		vector<t*> ret;
		ull hash = typeid(t).hash_code();
		for (auto &i : components)
			if (i.second->hash == hash)
			{
				ret.push_back((t *)i.first);
			}
		return ret;
	}

	void collide(game_object *go,glm::vec3 point, glm::vec3 normal)
	{
		// lock.lock();
		colLock.lock();
		colliding = true;
		if(destroyed){
			colLock.unlock();
			return;
		}
		for (auto &i : components)
		{
			i.first->onCollision(go,point,normal);
		}
		colliding = false;
		colLock.unlock();
		// if(this->destroyed){
		// 	this->destroy();
		// }
		// lock.unlock();
	}

	template <class t>
	t *addComponent()
	{
		// gameLock.lock();
		compInfo<t> ci = addComponentToAll(t());
		t *ret = (t *)ci.compPtr;
		// ci.CompItr->goComponents = &this->components;
		components.insert(std::make_pair(ret, ci.CompItr));
		ret->transform = this->transform;
		ret->transform->gameObject = this;
		ret->onStart();
		// gameLock.unlock();

		return ret;
	}

	template <class t>
	t *addComponent(const t &c)
	{
		// gameLock.lock();
		compInfo<t> ci = addComponentToAll(c);
		t *ret = (t *)ci.compPtr;
		// ci.CompItr->goComponents = &this->components;
		components.insert(std::make_pair(ret, ci.CompItr));
		ret->transform = this->transform;
		ret->transform->gameObject = this;
		ret->onStart();
		// gameLock.unlock();

		return ret;
	}

	template <class t>
	t *dupComponent(const t &c)
	{
				// gameLock.lock();
		compInfo<t> ci = addComponentToAll(c);
		t *ret = (t *)ci.compPtr;
		// ci.CompItr->goComponents = &this->components;
		components.insert(std::make_pair(ret, ci.CompItr));
		ret->transform = this->transform;
		ret->transform->gameObject = this;
		// gameLock.unlock();
		return ret;
	}
	template <class t>
	void removeComponent(t *c)
	{
		// gameLock.lock();
		auto toR = components.find(c);
		toR->first->onDestroy();
		toR->second->erase();
		components.erase(toR);
		// gameLock.unlock();
	}
	void removeComponent(component *c)
	{
		// gameLock.lock();
		auto toR = components.find(c);
		toR->first->onDestroy();
		toR->second->erase();
		components.erase(toR);
		// gameLock.unlock();
	}
	template <class t>
	void removeComponent()
	{
		// gameLock.lock();
		component *c = getComponent<t>();
		auto toR = components.find(c);
		toR->first->onDestroy();
		toR->second->erase();
		components.erase(toR);
		// gameLock.unlock();
	}

	void destroy()
	{
		toDestroy.insert(this);
		// lock.lock();
		// if(this->colliding){
		// 	this->destroyed = true;
		// 	lock.unlock();
		// 	return;
		// }
		// // if(this->destroyed){
		// // 	lock.unlock();
		// // 	return;
		// // }
		// for (auto &i : components)
		// {
		// 	i.first->onDestroy();
		// }
		// while(components.size() > 0){
		// 	components.begin()->second->erase();
		// 	components.erase(components.begin());
		// }
		// while (transform->getChildren().size() > 0)
		// {
		// 	transform->getChildren().front()->gameObject->destroy();
		// }
		// transform->_destroy();
		// lock.unlock();
		// this->_destroy();
	}

	game_object(Transform *t) : lock()
	{
		// gameLock.lock();
		destroyed = false;
		this->transform = t;
		t->gameObject = this;
		// gameLock.unlock();
	}
	game_object() : lock()
	{
		// gameLock.lock();
		destroyed = false;
		this->transform = new Transform(this);
		root->Adopt(this->transform);
		// gameLock.unlock();
	};
	game_object(const game_object &g) : lock()
	{
		// gameLock.lock();
		destroyed = false;
		this->transform = new Transform(*g.transform, this);
		g.transform->getParent()->Adopt(this->transform);
		for (auto &i : g.components)
		{
			i.first->_copy(this);
		}
		for (auto &i : this->components)
		{
			i.first->onStart();
		}
		// gameLock.unlock();
	}
	game_object(const game_object_proto &g) : lock()
	{
		destroyed = false;
		gameLock.lock();
		this->transform = new Transform(this);
		gameLock.unlock();
		for (auto &i : g.components)
		{
			i.first->_copy(this);
		}
		for (auto &i : this->components)
		{
			i.first->onStart();
		}
	}
	void _destroy()
	{
		lock.lock();
		if(this->colliding){
			this->destroyed = true;
			lock.unlock();
			return;
		}
		// if(this->destroyed){
		// 	lock.unlock();
		// 	return;
		// }
		for (auto &i : components)
		{
			i.first->onDestroy();
		}
		while(components.size() > 0){
			components.begin()->second->erase();
			components.erase(components.begin());
		}
		while (transform->getChildren().size() > 0)
		{
			transform->getChildren().front()->gameObject->destroy();
		}
		transform->_destroy();
		lock.unlock();
		// this->_destroy();
		delete this;
	}

	Transform *transform;
};

