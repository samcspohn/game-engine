#pragma once
#include <vector>
#include <map>
#include <list>
#include <typeinfo>
#include <glm/glm.hpp>
#include "concurrency.h"
// #include "plf_list.h"
#include <stdexcept>
// #include "game_object.h"
#include "fast_list.h"
#include "array_heap.h"
#include "helper1.h"
// #include <omp.h>
// #include <tbb/tbb.h>
// #include "tbb/blocked_range.h"
// #include "tbb/parallel_for.h"
// #include "tbb/partitioner.h"

#include "_serialize.h"

#include "Transform.h"
#include "fstream"
// #define ull unsigned long long
class game_object_proto_;

// bool compareTransform(Transform *t1, Transform *t2);
// class Transform2;
class collider;

struct collision
{
	glm::vec3 point;
	glm::vec3 normal;
	float penetration;
	collider *this_collider;
	collider *other_collider;
	game_object *g_o;
	collision(glm::vec3 _point,
			  glm::vec3 _normal,
			  collider *_this_collider,
			  collider *_other_collider,
			  game_object *_g_o) : point(_point), normal(_normal), this_collider(_this_collider), other_collider(_other_collider), g_o(_g_o) {}
	collision() {}
};

struct component
{
	friend game_object;

public:
	virtual void onStart();
	virtual void onDestroy();

	static bool _registerEngineComponent();
	virtual void onCollision(collision &);
	virtual void update();
	virtual void lateUpdate();
	virtual void fixedUpdate();
	virtual void init(int id);
	virtual void deinit(int id);
	virtual void editorUpdate();
	virtual void ser_edit(ser_mode x, YAML::Node &n) = 0;
	transform2 transform;
	int getThreadID();
	size_t getHash();
};
class componentStorageBase
{
public:
	size_t hash;
	bool h_update;
	bool h_lateUpdate;
	bool h_fixedUpdate;
	bool h_editorUpdate;
	timer update_timer;
	float update_t;
	float lateupdate_t;
	string name;
	mutex lock;
	virtual string getName() { return "component"; }
	bool hasUpdate() { return h_update; }
	bool hasLateUpdate() { return h_lateUpdate; }
	bool hasEditorUpdate() { return h_editorUpdate; }
	virtual void update(){};
	virtual void lateUpdate(){};
	virtual void fixedUpdate(){};
	virtual void editorUpdate(){};
	virtual component *get(int i) { return 0; }
	virtual bool getv(int i) { return false; }
	virtual void erase(int i) {}
	virtual int size() { return 0; };
	virtual unsigned int active() { return 0; };
	virtual void encode(YAML::Node &node, int i) = 0;
	virtual int decode(YAML::Node &node) = 0;
	virtual component *_decode(YAML::Node &node) = 0;
	virtual void sort(){};
	virtual void compress(){};
	virtual void clear() {}
	virtual int copy(int id) = 0;
	virtual void addComponent(game_object *g) = 0;
	virtual void addComponentProto(game_object_proto_ *g) = 0;
	virtual void floatingComponent(component *) = 0;
	virtual int copy(component *c) = 0;
};

template <typename t>
class componentStorage : public componentStorageBase
{
public:
	STORAGE<t> data;

	componentStorage(const char *_name);
	~componentStorage();
	unsigned int active()
	{
		return data.active();
	}
	void compress(){
		data.compress();
	}
	int size()
	{
		return data.size();
	}

	t *get(int i)
	{
		if (i >= data.size()){
			cout << "out of bounds: " << getName() << endl;
			return 0;
		}
		else
		{
			return &data.get(i);
		}
	}
	bool getv(int i)
	{
		if (i >= data.size()){
			cout << "out of bounds: " << getName() << endl;
			return 0;
		}
		else
			return data.getv(i);
	}
	void erase(int id)
	{
		data._delete(id);
	}

	string getName()
	{
		return name;
	}
	int copy(int id)
	{
		return data._new(data.get(id));
	}
	rolling_buffer _update_t;
	void update()
	{
		parallelfor(data.size(),
					{
						if (data.getv(i))
						{
							data.get(i).update();
						}
					});
	}
	rolling_buffer _lateupdate_t;
	void lateUpdate()
	{
		parallelfor(data.size(),
					{
						if (data.getv(i))
						{
							data.get(i).lateUpdate();
						}
					});
	}
	void fixedUpdate()
	{
		parallelfor(data.size(),
					{
						if (data.getv(i))
						{
							data.get(i).fixedUpdate();
						}
					});
	}
	void editorUpdate()
	{
		int size = data.size();
		for (int i = 0; i < size; i++)
		{
			if (data.getv(i))
			{
				data.get(i).editorUpdate();
			}
		}
	}
	void clear()
	{
		data.clear();
	}
	void addComponent(game_object *g);
	void addComponentProto(game_object_proto_ *g);
	void floatingComponent(component *c)
	{
		new (c) t();
	}

	int copy(component *c)
	{
		return data._new(*static_cast<t *>(c));
	}
	void encode(YAML::Node &node, int i)
	{
		try
		{
			data.get(i).ser_edit(ser_mode::write_mode, node);
		}
		catch (...)
		{
		}
	}
	int decode(YAML::Node &node)
	{
		int i = data._new();
		try
		{
			data.get(i).ser_edit(ser_mode::read_mode, node);
		}
		catch (...)
		{
		}
		return i;
	}
	virtual component *_decode(YAML::Node &node)
	{
		t *c = new t();
		c->ser_edit(ser_mode::read_mode, node);
		return c;
	}
};

#include <tbb/concurrent_unordered_map.h>
class Registry
{
public:
	tbb::concurrent_unordered_map<std::string, componentStorageBase *> meta;
	tbb::concurrent_unordered_map<size_t, componentStorageBase *> meta_types;
	std::mutex lock;

	void clear()
	{
		for (auto &i : meta)
		{
			i.second->clear();
		}
	}
	void registerComponentStorage(componentStorageBase *p)
	{
		// p->name = _name;
		meta.emplace(p->name, p);
		meta_types.emplace(p->hash, p);
	}
	void deregisterComponentStorage(componentStorageBase *p)
	{
		meta.unsafe_erase(p->name);
		meta_types.unsafe_erase(p->hash);
	}
	template <typename t>
	inline componentStorage<t> *registry()
	{
		return static_cast<componentStorage<t> *>(meta_types.at(typeid(t).hash_code()));
	}
	inline componentStorageBase *registry(size_t hash)
	{
		tbb::concurrent_unordered_map<size_t, componentStorageBase *>::iterator i;
		if ((i = meta_types.find(hash)) != meta_types.end())
			return i->second;
		return 0;
	}
};
extern Registry ComponentRegistry;

template <typename t>
componentStorage<t> *GetStorage()
{
	size_t hash = typeid(t).hash_code();
	return ComponentRegistry.registry<t>();
}
template <typename t>
int addComponentToRegistry(const t &c)
{
	componentStorage<t> *compStorage = GetStorage<t>();
	return compStorage->data._new(c);
}

template <typename t>
int addComponentToRegistry()
{
	componentStorage<t> *compStorage = GetStorage<t>();
	return compStorage->data._new();
}

void destroyAllComponents();
#define COMPONENT_LIST(x) static_cast<componentStorage<x> *>(ComponentRegistry.registry<x>())

#define REGISTER_COMPONENT(comp) componentStorage<comp> component_storage_##comp(#comp);

template <typename t>
componentStorage<t>::componentStorage(const char *_name)
{
	this->name = _name;
	this->hash = typeid(t).hash_code();
	this->h_update = typeid(&t::update) != typeid(&component::update);
	this->h_lateUpdate = typeid(&t::lateUpdate) != typeid(&component::lateUpdate);
	this->h_editorUpdate = typeid(&t::editorUpdate) != typeid(&component::editorUpdate);
	this->h_fixedUpdate = typeid(&t::fixedUpdate) != typeid(&component::fixedUpdate);
	std::cout << "register component " << name << std::endl;
	ComponentRegistry.registerComponentStorage(this);
}

template <typename t>
componentStorage<t>::~componentStorage()
{
	std::cout << "de-register component " << name << std::endl;
	ComponentRegistry.deregisterComponentStorage(this);
}
