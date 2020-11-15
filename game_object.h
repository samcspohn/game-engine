#pragma once
#include "Component.h"
#include "Transform.h"
#include <map>
#include <set>
#include <unordered_set>
#include <list>
#include "plf_list.h"
#include "fast_list.h"
#include "serialize.h"

using namespace std;

class _renderer;
class game_object_proto;
#define protoListRef typename std::list<game_object_proto*>::iterator
#define protoList std::list<game_object_proto*>

void registerProto(game_object_proto* p);
void deleteProtoRef(protoListRef r);

class game_object_proto{
public:
	game_object_proto(){
		// ref = registerProto(this);
	}
	map<component *, ull> components;
	protoListRef ref;

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
	~game_object_proto(){
		for(auto& i : components)
			delete i.first;
		deleteProtoRef(ref);
	}
	SER_HELPER(){
		ar & components;
	}
};

void saveProto(OARCHIVE& oa);
void loadProto(IARCHIVE& ia);

class game_object;

extern tbb::concurrent_unordered_set<game_object*> toDestroy;
extern std::list<game_object_proto*> prototypeRegistry;
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
	friend void rebuildGameObject(componentStorageBase*, int);
	friend void save_game(const char*);
	friend void load_game(const char*);
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
		compInfo ci = addComponentToRegistry<t>();
		t *ret = (t *)ci.compPtr;
		// ci.CompItr->goComponents = &this->components;
		components.insert(std::make_pair(ret, ci.CompItr));
		ret->transform = this->transform;
		ret->transform->setGameObject(this);
		ret->onStart();
		// gameLock.unlock();

		return ret;
	}

	template <class t>
	t *addComponent(const t &c)
	{
		// gameLock.lock();
		compInfo ci = addComponentToRegistry(c);
		t *ret = (t *)ci.compPtr;
		// ci.CompItr->goComponents = &this->components;
		components.insert(std::make_pair(ret, ci.CompItr));
		ret->transform = this->transform;
		ret->transform->setGameObject(this);
		ret->onStart();
		// gameLock.unlock();

		return ret;
	}

	template <class t>
	t *dupComponent(const t &c)
	{
				// gameLock.lock();
		compInfo ci = addComponentToRegistry(c);
		t *ret = (t *)ci.compPtr;
		// ci.CompItr->goComponents = &this->components;
		components.insert(std::make_pair(ret, ci.CompItr));
		ret->transform = this->transform;
		ret->transform->setGameObject(this);
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
	}

	game_object(transform2 t) : lock()
	{
		// gameLock.lock();
		destroyed = false;
		this->transform = t;
		t->setGameObject( this );
		// gameLock.unlock();
	}
	game_object() : lock()
	{
		// gameLock.lock();
		destroyed = false;
		// this->transform = new Transform(this);
		this->transform = Transforms._new();
		// root->Adopt(this->transform);
		// gameLock.unlock();
	};
	game_object(game_object &g) : lock()
	{
		// gameLock.lock();
		destroyed = false;
		// this->transform = new Transform(*g.transform, this);
		// g.transform->getParent()->Adopt(this->transform);

		this->transform = Transforms._new();// new Transform(this);
		this->transform->init(g.transform, this);
		g.transform.getParent().adopt(this->transform);
		// g.transform.getParent().adopt(this->transform);

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
		this->transform = Transforms._new();// new Transform(this);
		this->transform->init(this);
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
			transform->getChildren().front()->gameObject()->destroy();
		}
		transform->_destroy();
		lock.unlock();
		// this->_destroy();
		delete this;
	}

	transform2 transform;
};
