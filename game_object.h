#pragma once
#include "Transform.h"
#include <map>
#include "Component.h"
#include <set>
#include "plf_list.h"
#include "fast_list.h"
//#include "physics.h"
using namespace std;

mutex removeLock;
std::set<compItr *> toRemove;
mutex destroyLock;
std::set<Transform *> toDestroy;

class _renderer;

class game_object
{

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

	void collide(game_object *go)
	{
		for (auto &i : components)
		{
			i.first->onCollision(go);
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
		removeLock.lock();
		auto toR = components.find(c);
		if (toRemove.find(toR->second) != toRemove.end())
		{
			cout << "already removed" << endl;
			throw;
		}
		toRemove.insert(toR->second);
		toR->first->onDestroy();
		removeLock.unlock();
		// components.erase(toR);
	}
	void removeComponent(component *c)
	{
		removeLock.lock();
		auto toR = components.find(c);
		if (toRemove.find(toR->second) != toRemove.end())
		{
			cout << "already removed" << endl;
			throw;
		}
		toRemove.insert(toR->second);
		toR->first->onDestroy();
		removeLock.unlock();
		// components.erase(toR);
	}
	template <class t>
	void removeComponent()
	{
		removeLock.lock();
		component *c = getComponent<t>();
		auto toR = components.find(c);
		if (toRemove.find(toR->second) != toRemove.end())
		{
			cout << "already removed" << endl;
			throw;
		}
		toRemove.insert(toR->second);
		toR->first->onDestroy();
		removeLock.unlock();
	}

	void destroy()
	{
		if (!destroyed)
		{

			destroyLock.lock();
			for(auto& i : components)
			// while (components.size() > 0)
			{
				// removeLock.lock();
				// auto toR = components.find(i.first);
				// if (toRemove.find(toR->second) != toRemove.end())
				// {
				// 	cout << "already removed" << endl;
				// 	throw;
				// }
				// toRemove.insert(toR->second);
				// removeLock.unlock();
				// toR->first->onDestroy();
				removeComponent(i.first);
			}
			if (toDestroy.find(transform) != toDestroy.end())
			{
				cout << "already destroyed" << endl;
				throw;
			}
			toDestroy.insert(transform);
			destroyLock.unlock();
			for (auto &i : transform->getChildren())
			{
				i->gameObject->destroy();
			}
		}
	}

	game_object(Transform *t)
	{
		destroyed = false;
		this->transform = t;
		t->gameObject = this;
	}
	game_object()
	{
		destroyed = false;
		this->transform = new Transform(this);
		root->Adopt(this->transform);
	};
	game_object(const game_object &g)
	{
		destroyed = false;
		this->transform = new Transform(*g.transform, this);
		g.transform->getParent()->Adopt(this->transform);
		for (auto &i : g.components)
		{
			i.second->getComponent()->_copy(this);
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
