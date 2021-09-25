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
	collider *this_collider;
	collider *other_collider;
	game_object *g_o;
	collision(glm::vec3 _point,
			  glm::vec3 _normal,
			  collider *_this_collider,
			  collider *_other_collider,
			  game_object *_g_o) : point(_point), normal(_normal), this_collider(_this_collider), other_collider(_other_collider), g_o(_g_o) {}
};

class component
{
	friend game_object;

public:
	virtual void onStart();
	virtual void onDestroy();

	static bool _registerEngineComponent();
	virtual void onCollision(collision &);
	virtual void update();
	virtual void lateUpdate();
	virtual void init(int id);
	virtual void deinit(int id);

	// virtual void onEdit() = 0;
	// template<class Archive>
	virtual void ser_edit(ser_mode x, YAML::Node &n) = 0;
	// virtual void ser_edit(ser_mode x) = 0;
	transform2 transform;
	int getThreadID();
	size_t getHash();

	SER_HELPER()
	{
		ar;
		// ar &transform;
	}
};
REGISTER_BASE(component)

class componentStorageBase
{
public:
	size_t hash;
	bool h_update;
	bool h_lateUpdate;
	timer update_timer;
	float update_t;
	float lateupdate_t;
	string name;
	mutex lock;
	virtual string getName() { return "component"; }
	bool hasUpdate() { return h_update; }
	bool hasLateUpdate() { return h_lateUpdate; }
	virtual void update(){};
	virtual void lateUpdate(){};
	virtual component *get(int i) { return 0; }
	virtual bool getv(int i) { return false; }
	virtual void erase(int i) {}
	virtual int size() { return 0; };
	virtual unsigned int active() { return 0; };
	virtual void encode(YAML::Node &node, int i) = 0;
	virtual int decode(YAML::Node &node) = 0;
	virtual component *_decode(YAML::Node &node) = 0;
	virtual void sort(){};
	virtual void clear() {}
	virtual int copy(int id) = 0;
	// virtual string ser(){};
};

// std::ostream & operator<<(std::ostream &os, const componentStorageBase &base)
// {
//     return os << base.name << base.h_update << base.h_lateUpdate << base.ser();
// }
REGISTER_BASE(componentStorageBase)

template <typename t>
class componentStorage : public componentStorageBase
{
public:
	// deque_heap<t> data;
	STORAGE<t> data;

	componentStorage()
	{
	}
	unsigned int active()
	{
		return data.active();
	}
	int size()
	{
		return data.size();
	}

	t *get(int i)
	{
		if (i >= data.size())
			return 0;
		else
			return &data.get(i);
	}
	bool getv(int i)
	{
		if (i >= data.size())
			return 0;
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
		update_timer.start();
		if (true) //_update_t.getAverageValue() > 1.f)
		{
			parallelfor(data.size(),
						{
							if (data.getv(i))
							{
								data.get(i).update();
							}
						});
		}
		else
		{
			int size = data.size();
			for (int i = 0; i < size; i++)
			{
				if (data.getv(i))
				{
					data.get(i).update();
				}
			}
		}
		_update_t.add(update_timer.stop());
	}
	rolling_buffer _lateupdate_t;
	void lateUpdate()
	{
		update_timer.start();
		if (_lateupdate_t.getAverageValue() > 1.f)
		{
			parallelfor(data.size(),
						{
							if (data.getv(i))
							{
								data.get(i).lateUpdate();
							}
						});
		}
		else
		{
			int size = data.size();
			for (int i = 0; i < size; i++)
			{
				if (data.getv(i))
				{
					data.get(i).lateUpdate();
				}
			}
		}
		_lateupdate_t.add(update_timer.stop());
	}
	void clear()
	{
		data.clear();
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
	// string ser(){
	// 	stringstream ss;
	// 	ss << boost::serialization:: data;
	// 	return string(ss.str());
	// }
};

struct componentMetaBase
{
	virtual void addComponent(game_object *g);
	virtual void addComponentProto(game_object_proto_ *g);
	// virtual void removeComponent(game_object *g);
	// virtual void removeComponentProto(game_object_proto_ *g);
	virtual void floatingComponent(component *) = 0;
	virtual int copy(int id) = 0;
	virtual int copy(component *c) = 0;
	virtual componentStorageBase *getStorage() = 0;
};
template <typename t>
struct componentMeta : public componentMetaBase
{
	componentStorage<t> storage;
};

class Registry
{
public:
	// std::map<size_t, unique_ptr<componentStorageBase>> components;
	std::map<std::string, shared_ptr<componentMetaBase>> meta;
	std::map<size_t, shared_ptr<componentMetaBase>> meta_types;
	std::mutex lock;

	void clear()
	{
		// for (auto &i : components)
		// {
		// 	delete i.second;
		// 	// i.second->clear();
		// }
		// components.clear();
		// gameEngineComponents.clear();
		// gameComponents.clear();
	}
	template <typename t>
	inline componentStorage<t> *registry()
	{
		return static_cast<componentStorage<t> *>(&(static_cast<componentMeta<t> *>(meta_types[typeid(t).hash_code()].get())->storage));
	}
	inline componentStorageBase *registry(size_t hash)
	{
		return meta_types.at(hash)->getStorage();
	}

	componentMetaBase *getByType(size_t type)
	{
		return meta_types[type].get();
	}
};

// extern std::map<ull, componentStorageBase *> componentRegistry;
// extern std::set<componentStorageBase *> gameEngineComponents;
// extern std::set<componentStorageBase *> gameComponents;
// extern std::mutex componentLock;
extern Registry ComponentRegistry;

template <typename t>
componentStorage<t> *GetStorage()
{
	size_t hash = typeid(t).hash_code();
	return ComponentRegistry.registry<t>();
}

template <typename t>
componentMeta<t> *registerComponent()
{
	size_t hash = typeid(t).hash_code();
	ComponentRegistry.meta_types.emplace(hash, make_shared<componentMeta<t>>());
	ComponentRegistry.meta.emplace(string(typeid(t).name()), ComponentRegistry.meta_types.at(hash));

	// ComponentRegistry.components[hash] = make_unique<componentStorage<t>>();
	componentStorageBase *csb = ComponentRegistry.registry<t>();
	csb->name = typeid(t).name();
	csb->h_update = typeid(&t::update) != typeid(&component::update);
	csb->h_lateUpdate = typeid(&t::lateUpdate) != typeid(&component::lateUpdate);
	csb->hash = hash;
	// if (t::_registerEngineComponent())
	// 	ComponentRegistry.gameEngineComponents.insert(pair(hash, ComponentRegistry.components[hash]));
	// else
	// ComponentRegistry.gameComponents.insert(pair(hash, ComponentRegistry.components[hash]));
	return static_cast<componentMeta<t>*>(ComponentRegistry.meta_types.at(hash).get());
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

// void save_game(const char *filename);

// void load_game(const char *filename);

void destroyAllComponents();
#define COMPONENT_LIST(x) static_cast<componentStorage<x> *>(ComponentRegistry.registry<x>())

#define REGISTER_COMPONENT(comp)                               \
	template <>                                                \
	struct componentMeta<comp> : public componentMetaBase      \
	{                                                          \
		static componentMeta<comp> const *c;                   \
		void addComponent(game_object *g)                      \
		{                                                      \
			g->_addComponent<comp>();                          \
		}                                                      \
		void addComponentProto(game_object_proto_ *g)          \
		{                                                      \
			g->addComponent<comp>();                           \
		}                                                      \
		void floatingComponent(component *c)                   \
		{                                                      \
			new (c) comp();                                    \
		}                                                      \
		componentStorageBase *getStorage()                     \
		{                                                      \
			return &storage;                                   \
		}                                                      \
		int copy(int id)                                       \
		{                                                      \
			return storage.copy(id);                           \
		}                                                      \
		int copy(component *c)                                 \
		{                                                      \
			return storage.data._new(*static_cast<comp *>(c)); \
		}                                                      \
		componentStorage<comp> storage;                        \
	};                                                         \
	componentMeta<comp> const *componentMeta<comp>::c = registerComponent<comp>();
// template<>
// componentMeta<comp>::g = registerComponent<comp>();
