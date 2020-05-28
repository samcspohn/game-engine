#pragma once
#include "Transform.h"
#include <map>
#include "Component.h"
#include <set>
#include <unordered_set>
#include "plf_list.h"
#include "fast_list.h"
//#include "physics.h"
using namespace std;

plf::list<compItr *> toRemove;
plf::list<Transform *> toDestroy;

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
		this->components.insert(std::pair(ret,typeid(t).hash_code()));
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


class game_object
{
	mutex lock;
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
		for (auto &i : components)
		{
			i.first->onCollision(go,point,normal);
		}
	}

	template <class t>
	t *addComponent()
	{
		compInfo<t> ci = addComponentToAll(t());
		t *ret = (t *)ci.compPtr;
		ci.CompItr->goComponents = &this->components;
		components.insert(std::make_pair(ret, ci.CompItr));
		ret->transform = this->transform;
		ret->transform->gameObject = this;
		ret->onStart();

		return ret;
	}

	template <class t>
	t *addComponent(const t &c)
	{
		compInfo<t> ci = addComponentToAll(c);
		t *ret = (t *)ci.compPtr;
		ci.CompItr->goComponents = &this->components;
		components.insert(std::make_pair(ret, ci.CompItr));
		ret->transform = this->transform;
		ret->transform->gameObject = this;
		ret->onStart();

		return ret;
	}

	template <class t>
	t *dupComponent(const t &c)
	{
		compInfo<t> ci = addComponentToAll(c);
		t *ret = (t *)ci.compPtr;
		ci.CompItr->goComponents = &this->components;
		components.insert(std::make_pair(ret, ci.CompItr));
		ret->transform = this->transform;
		ret->transform->gameObject = this;
		return ret;
	}
	template <class t>
	void removeComponent(t *c)
	{
		gameLock.lock();
		auto toR = components.find(c);
		// if (toRemove.find(toR->second) != toRemove.end())
		// {
		// 	cout << "already removed" << endl;
		// 	throw;
		// }
		toRemove.push_back(toR->second);
		toR->first->onDestroy();
		gameLock.unlock();
		// components.erase(toR);
	}
	void removeComponent(component *c)
	{
		gameLock.lock();
		auto toR = components.find(c);
		toRemove.push_back(toR->second);
		toR->first->onDestroy();
		gameLock.unlock();
		// components.erase(toR);
	}
	template <class t>
	void removeComponent()
	{
		gameLock.lock();
		component *c = getComponent<t>();
		auto toR = components.find(c);
		toRemove.push_back(toR->second);
		toR->first->onDestroy();
		gameLock.unlock();
	}

	void destroy()
	{
		gameLock.lock();
		if (!destroyed)
		{
			for (auto &i : components)
			{
				auto toR = components.find(i.first);
				toRemove.push_back(toR->second);
				toR->first->onDestroy();
			}
			toDestroy.push_back(transform);
			for (auto &i : transform->getChildren())
			{
				i->gameObject->destroy();
			}
		}
		gameLock.unlock();
	}

	game_object(Transform *t) : lock()
	{
		gameLock.lock();
		destroyed = false;
		this->transform = t;
		t->gameObject = this;
		gameLock.unlock();
	}
	game_object() : lock()
	{
		gameLock.lock();
		destroyed = false;
		this->transform = new Transform(this);
		root->Adopt(this->transform);
		gameLock.unlock();
	};
	game_object(const game_object &g) : lock()
	{
		gameLock.lock();
		destroyed = false;
		this->transform = new Transform(*g.transform, this);
		g.transform->getParent()->Adopt(this->transform);
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
	game_object(const game_object_proto &g) : lock()
	{
		gameLock.lock();
		destroyed = false;
		this->transform = new Transform(this);
		gameLock.unlock();
		for (auto &i : g.components)
		{
			i.first->_copy(this);
		}
		for (auto &i : this->components)
		{
			i.second->getComponent()->onStart();
		}
	}
	void _destroy()
	{
		delete this;
	}

	Transform *transform;
};

game_object *rootGameObject;
