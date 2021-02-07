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
#include "tbb/blocked_range.h"
#include "tbb/parallel_for.h"
#include "tbb/partitioner.h"

#include "serialize.h"

#include "Transform.h"
#include "fstream"
#define ull unsigned long long
class game_object_proto_;

// bool compareTransform(Transform *t1, Transform *t2);
// class Transform2;
class component
{
	friend game_object;

public:
	virtual void onStart();
	virtual void onDestroy();

	virtual bool _registerEngineComponent();
	virtual void onCollision(game_object *go, glm::vec3 point, glm::vec3 normal);
	virtual void update();
	virtual void lateUpdate();

	virtual void onEdit() = 0;
	virtual void _copy(game_object *go) = 0;
	transform2 transform;
	int getThreadID();
	ull getHash();

	SER_HELPER()
	{
		ar;
		// ar &transform;
	}
};
REGISTER_BASE(component)

struct compItr
{
	ull hash;
	// map<component *, compItr *> *goComponents;
	virtual void erase();
	virtual component *getComponent();
};

template <typename t>
struct compItr_ : public compItr
{
	typename deque_heap<t>::ref id;
	deque_heap<t> *l;
	void erase()
	{
		//	    delete *id;
		// (&(id.data()))->onDestroy();
		// goComponents->erase(getComponent());
		l->_delete(id);
		delete this;
	}
	typename deque_heap<t>::ref get()
	{
		return id;
	}
	component *getComponent()
	{
		return (component *)&(*id);
	}
	compItr_(typename deque_heap<t>::ref _id, deque_heap<t> *_l) : id(_id), l(_l) {}
	compItr_() {}
};

struct compInfo
{
	component *compPtr;
	compItr *CompItr;
};

extern tbb::affinity_partitioner update_ap;

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
	virtual int size() { return 0; };
	virtual unsigned int active() { return 0; };
	virtual void sort(){};
	virtual compInfo getInfo(int i) { return compInfo(); };
	virtual void clear(){}
	// virtual string ser(){};

	friend class boost::serialization::access;
	template <class Archive>
	void serialize(Archive &ar, const unsigned int /* file_version */)
	{
		ar &name &h_update &h_lateUpdate;
	}
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
	deque_heap<t> data;

	componentStorage()
	{
	}
	unsigned int active()
	{
		return data.active;
	}
	int size()
	{
		return data.size();
	}

	component *get(int i)
	{
		return (component *)&(data.data[i]);
	}
	bool getv(int i)
	{
		return data.valid[i];
	}
	compInfo getInfo(int i)
	{
		this->lock.lock();
		typename deque_heap<t>::ref id = data.getRef(i);
		this->lock.unlock();
		// new (&(*id)) t(c);
		// *id = std::move(c);

		compInfo ret;
		ret.compPtr = &(*id);
		ret.CompItr = new compItr_<t>(id, &data);
		ret.CompItr->hash = typeid(t).hash_code();
		return ret;
	}
	compInfo getInfo(typename deque_heap<t>::ref id)
	{
		compInfo ret;
		ret.compPtr = &(*id);
		ret.CompItr = new compItr_<t>(id, &data);
		ret.CompItr->hash = typeid(t).hash_code();
		return ret;
	}
	string getName()
	{
		return name;
	}

	void update()
	{
		update_timer.start();
		if (update_t > 0.1f)
		{
			_parallel_for(data, [&](int i) {
				if (data.valid[i])
				{
					data.data[i].update();
				}
			});
		}
		else
		{
			int size = data.size();
			for (int i = 0; i < size; i++)
			{
				if (data.valid[i])
				{
					data.data[i].update();
				}
			}
		}
		update_t = update_timer.stop();
	}
	void lateUpdate()
	{
		update_timer.start();
		if (lateupdate_t > 0.1f)
		{
			_parallel_for(data, [&](int i) {
				if (data.valid[i])
				{
					data.data[i].lateUpdate();
				}
			});
		}
		else
		{
			int size = data.size();
			for (int i = 0; i < size; i++)
			{
				if (data.valid[i])
				{
					data.data[i].lateUpdate();
				}
			}
		}
		lateupdate_t = update_timer.stop();
	}
	void clear(){
		data.clear();
	}

	friend class boost::serialization::access;

	template <class Archive>
	void serialize(Archive &ar, const unsigned int /* file_version */) {}
	void serialize(OARCHIVE &ar, const unsigned int)
	{
		string s;
		vector<int> transforms;
		for (auto &i : data.data)
		{
			transforms.push_back(i.transform.id);
		}
		{
			stringstream ss;
			OARCHIVE _ar(ss);
			_ar << data;
			s = ss.str();
		}
		ar << boost::serialization::base_object<componentStorageBase>(*this) << transforms << s;
	}
	void serialize(IARCHIVE &ar, const unsigned int)
	{
		string s;
		vector<int> transforms;
		ar >> boost::serialization::base_object<componentStorageBase>(*this) >> transforms >> s;
		{
			stringstream ss{s};
			try
			{
				IARCHIVE _ar(ss);
				_ar >> data;
			}
			catch (exception e)
			{
				data.data.resize(transforms.size());
				cout << e.what() << endl;
			}
			// s = ss.str();
		}
		for (int i = 0; i < transforms.size(); ++i)
		{
			data.data[i].transform.id = transforms[i];
		}
	}
	// string ser(){
	// 	stringstream ss;
	// 	ss << boost::serialization:: data;
	// 	return string(ss.str());
	// }
};

struct component_meta_base
{
};
template <typename t>
struct component_meta : public component_meta_base
{
};

struct componentMetaBase
{
	virtual void addComponent(game_object *g);
	virtual void addComponentProto(game_object_proto_ *g);
	virtual void floatingComponent(component *) = 0;
};
template <typename t>
struct componentMeta : public componentMetaBase
{
};

class Registry
{
public:
	std::map<size_t, componentStorageBase *> components;
	std::map<size_t, componentStorageBase *> gameEngineComponents;
	std::map<size_t, componentStorageBase *> gameComponents;
	std::map<std::string, componentMetaBase *> meta;
	std::map<size_t, componentMetaBase *> meta_types;
	std::mutex lock;

	void clear(){
		for(auto& i : components){
			i.second->clear();
		}
	}

	friend class boost::serialization::access;

	template <class Archive>
	void serialize(Archive &ar, const unsigned int /* file_version */)
	{
		ar &components &gameEngineComponents &gameComponents;
	}
	template <typename t>
	inline componentStorage<t> *registry()
	{
		return static_cast<componentStorage<t> *>(components[typeid(t).hash_code()]);
	}

	componentMetaBase *getByType(ull type)
	{
		return meta_types[type];
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
	if (ComponentRegistry.components.find(hash) == ComponentRegistry.components.end())
	{
		ComponentRegistry.lock.lock();
		if (ComponentRegistry.components.find(hash) == ComponentRegistry.components.end())
		{
			componentStorageBase *csb = (componentStorageBase *)(new componentStorage<t>());
			ComponentRegistry.components[hash] = csb;
			csb->name = typeid(t).name();
			csb->h_update = typeid(&t::update) != typeid(&component::update);
			csb->h_lateUpdate = typeid(&t::lateUpdate) != typeid(&component::lateUpdate);
			csb->hash = hash;
			if (t()._registerEngineComponent())
				ComponentRegistry.gameEngineComponents.insert(pair(hash, ComponentRegistry.components[hash]));
			else
				ComponentRegistry.gameComponents.insert(pair(hash, ComponentRegistry.components[hash]));
		}
		ComponentRegistry.lock.unlock();
	}
	return static_cast<componentStorage<t> *>(ComponentRegistry.components[hash]);
}

template <typename t>
component_meta<t> registerComponent()
{
	ComponentRegistry.meta_types.emplace(pair(typeid(t).hash_code(), (componentMetaBase *)(new componentMeta<t>())));
	ComponentRegistry.meta.emplace(pair(string(typeid(t).name()), ComponentRegistry.meta_types.at(typeid(t).hash_code())));
	GetStorage<t>();
	return component_meta<t>();
}
template <typename t>
inline compInfo addComponentToRegistry(const t &c)
{
	componentStorage<t> *compStorage = GetStorage<t>();
	typename deque_heap<t>::ref id = compStorage->data._new(c);
	return compStorage->getInfo(id);
}

template <typename t>
inline compInfo addComponentToRegistry()
{
	componentStorage<t> *compStorage = GetStorage<t>();
	typename deque_heap<t>::ref id = compStorage->data._new();
	return compStorage->getInfo(id);
}

// void save_game(const char *filename);

// void load_game(const char *filename);

void destroyAllComponents();
#define COMPONENT_LIST(x) static_cast<componentStorage<x> *>(ComponentRegistry.components[typeid(x).hash_code()])

#define COPY(component_type)                     \
	void _copy(game_object *go)                  \
	{                                            \
		go->dupComponent(component_type(*this)); \
	}

#define REGISTER_COMPONENT(comp)                          \
	BOOST_CLASS_EXPORT(comp)                              \
	BOOST_CLASS_EXPORT(componentStorage<comp>)            \
	template <>                                           \
	struct componentMeta<comp> : public componentMetaBase \
	{                                                     \
		static component_meta<comp> const &c;             \
		void addComponent(game_object *g)                 \
		{                                                 \
			g->addComponent<comp>();                      \
		}                                                 \
		void addComponentProto(game_object_proto_ *g)     \
		{                                                 \
			g->addComponent<comp>();                      \
		}                                                 \
		void floatingComponent(component *c)              \
		{                                                 \
			new (c) comp();                               \
		}                                                 \
	};                                                    \
	component_meta<comp> const &componentMeta<comp>::c = registerComponent<comp>();
// template<>
// componentMeta<comp>::g = registerComponent<comp>();
