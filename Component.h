#pragma once
#include <vector>
#include <map>
#include <list>
#include <typeinfo>
#include <glm/glm.hpp>
#include "concurrency.h"
#include "plf_list.h"
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
// class game_object;

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
	// virtual void _update(int index, unsigned int _start, unsigned int _end);
	// virtual void _lateUpdate(int index, unsigned int _start, unsigned int _end);
	virtual void _copy(game_object *go) = 0;
	transform2 transform;
	int getThreadID();
	ull getHash();

	// friend std::ostream & operator<<(std::ostream &os, const component &c);
	// friend class boost::serialization::access;

	// template<class Archive>
	// void serialize(Archive & ar, const unsigned int /* file_version */){
	//     ar & transform;
	// }
	// virtual boost::archive::text_oarchive &forceSerialize(boost::archive::text_oarchive &ar) const = 0;
	// friend boost::archive::text_oarchive &operator<<(boost::archive::text_oarchive &os, const component &c);
	friend class boost::serialization::access;

	template <class Archive>
	inline void serialize(Archive &ar, const unsigned int /* file_version */)
	{
		ar &transform;
	}
};
BOOST_SERIALIZATION_ASSUME_ABSTRACT(component)

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
BOOST_SERIALIZATION_ASSUME_ABSTRACT(componentStorageBase)

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

	friend class boost::serialization::access;

	template <class Archive>
	void serialize(Archive &ar, const unsigned int /* file_version */)
	{
		ar &boost::serialization::base_object<componentStorageBase>(*this) & data;
	}
	// string ser(){
	// 	stringstream ss;
	// 	ss << boost::serialization:: data;
	// 	return string(ss.str());
	// }
};

class Registry
{
public:
	std::map<size_t, componentStorageBase *> components;
	std::map<size_t, componentStorageBase *> gameEngineComponents;
	std::map<size_t, componentStorageBase *> gameComponents;
	std::mutex lock;

	friend class boost::serialization::access;

	template <class Archive>
	void serialize(Archive &ar, const unsigned int /* file_version */)
	{
		ar &components &gameEngineComponents &gameComponents;
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
