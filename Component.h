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
	virtual void forceSerialize() = 0;
	friend boost::archive::text_oarchive  & operator<<(boost::archive::text_oarchive &os, const component &c);
    friend class boost::serialization::access;

	template<class Archive>
    void serialize(Archive & ar, const unsigned int /* file_version */){
        ar & transform;
	}

};
SERIALIZE_STREAM(component) << o.transform SSE;


// std::ostream & operator<<(std::ostream &os, const component &c)
// {
//     return os << ' ' << c.transform;
// }


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

template <typename t>
struct compInfo
{
	component *compPtr;
	compItr *CompItr;
};

extern tbb::affinity_partitioner update_ap;

class componentStorageBase
{
public:
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
	virtual component *get(int i) {return 0;}
	virtual bool getv(int i) {return false;}
	virtual int size(){ return 0;};
	virtual unsigned int active(){return 0;};
	virtual void sort(){};
};

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
};

extern std::map<ull, componentStorageBase *> allcomponents;
extern std::set<componentStorageBase *> gameEngineComponents;
extern std::set<componentStorageBase *> gameComponents;
extern std::mutex componentLock;
template <typename t>
inline compInfo<t> addComponentToAll(const t &c)
{
	ull hash = typeid(t).hash_code();
	if (allcomponents.find(hash) == allcomponents.end())
	{
		componentLock.lock();
		if (allcomponents.find(hash) == allcomponents.end())
		{
			componentStorageBase *csb = (componentStorageBase *)(new componentStorage<t>());
			allcomponents[hash] = csb;
			csb->name = typeid(t).name();
			csb->h_update = typeid(&t::update) != typeid(&component::update);
			csb->h_lateUpdate = typeid(&t::lateUpdate) != typeid(&component::lateUpdate);
			if (((component *)&c)->_registerEngineComponent())
				gameEngineComponents.insert(allcomponents[hash]);
			else
				gameComponents.insert(allcomponents[hash]);
		}
		componentLock.unlock();
	}
	componentStorage<t> *compStorage = static_cast<componentStorage<t> *>(allcomponents[hash]);
	compStorage->lock.lock();
	typename deque_heap<t>::ref id = compStorage->data._new();
	compStorage->lock.unlock();
	new(&(*id)) t(c);
	// *id = std::move(c);

	compInfo<t> ret;
	ret.compPtr = &(*id);
	ret.CompItr = new compItr_<t>(id, &compStorage->data);
	ret.CompItr->hash = hash;
	return ret;
}

void destroyAllComponents();
#define COMPONENT_LIST(x) static_cast<componentStorage<x> *>(allcomponents[typeid(x).hash_code()])

#define COPY(component_type)                     \
	void _copy(game_object *go)                  \
	{                                            \
		go->dupComponent(component_type(*this)); \
	}
