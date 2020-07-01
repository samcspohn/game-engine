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
#include <omp.h>

#define ull unsigned long long
class game_object;

// bool compareTransform(Transform *t1, Transform *t2);
class Transform;
class component
{
	friend game_object;
public:
	virtual void onStart();
	virtual void onDestroy();

	virtual bool _registerEngineComponent();
	virtual void onCollision(game_object *go, glm::vec3 point,  glm::vec3 normal);
	virtual void update();
	virtual void lateUpdate();
	// virtual void _update(int index, unsigned int _start, unsigned int _end);
	// virtual void _lateUpdate(int index, unsigned int _start, unsigned int _end);
	virtual void _copy(game_object *go) = 0;
	Transform* transform;
	int getThreadID();
	ull getHash();
};

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

class componentStorageBase
{

public:
	rolling_buffer duration;
	timer T;
	bool h_update;
	bool h_lateUpdate;
	string name;
	mutex lock;
	bool hasUpdate(){return h_update;}
	bool hasLateUpdate(){return h_lateUpdate;}
	virtual void update(){};
	virtual void lateUpdate(){};
	virtual void update(int index, int size) {}
	virtual void lateUpdate(int index, int size) {}
	virtual component* get(int i){}
	virtual bool getv(int i){}
	virtual int size(){};
	virtual void sort(){};
};



template <typename t>
class componentStorage : public componentStorageBase
{
public:
	deque_heap<t> data;

	componentStorage(){
		duration = rolling_buffer(30);
	}
	int size()
	{
		return data.size();
	}

	// void update()
	// {
	// 	((component *)&(data.data.front()))->_update(0, 0, data.size());
	// }

	virtual component* get(int i){
		return (component*)&(data.data[i]);
	}
	virtual bool getv(int i){
		return data.valid[i];
	}

	void update(int index, int size)
	{
		unsigned int _start = (unsigned int)((float)size / (float)concurrency::numThreads * (float)index);
		unsigned int _end = (unsigned int)((float)size / (float)concurrency::numThreads * (float)(index + 1));
		if (index == concurrency::numThreads - 1)
			_end = size;
		// ((component *)&(data.data.front()))->_update(index, _start, _end);
		auto start = data.data.begin() + _start;
		auto valid = data.valid.begin() + _start;
		auto end = data.data.begin() + _end;
		for(; start != end; ++start, ++valid){
			if(*valid){
				start->update();
			}
		}

	}

	void update()
	{
		int size = this->size();
		T.start();
		if(duration.getAverageValue() > 1.0){
			#pragma omp parallel for schedule(guided)
			for(int i = 0; i < size; ++i){
				if(data.valid[i]){
					data.data[i].update();
				}
			}
		}else{
			for(int i = 0; i < size; ++i){
				if(data.valid[i]){
					data.data[i].update();
				}
			}
		}
		duration.add(T.stop());
	}
	void lateUpdate(){
		int size = this->size();
		#pragma omp parallel for
		for(int i = 0; i < size; ++i){
			if(data.valid[i]){
				data.data[i].lateUpdate();
			}
		}

	}
	void lateUpdate(int index, int size)
	{
		unsigned int _start = (unsigned int)((float)size / (float)concurrency::numThreads * (float)index);
		unsigned int _end = (unsigned int)((float)size / (float)concurrency::numThreads * (float)(index + 1));
		if (index == concurrency::numThreads - 1)
			_end = size;
		// ((component *)&(data.data.front()))->_lateUpdate(index, _start, _end);
		auto start = data.data.begin() + _start;
		auto valid = data.valid.begin() + _start;
		auto end = data.data.begin() + _end;
		for(; start != end; ++start, ++valid){
			if(*valid){
				start->lateUpdate();
			}
		}
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
			componentStorageBase * csb = (componentStorageBase *)(new componentStorage<t>());
			// allcomponents[hash] = (componentStorageBase *)(new componentStorage<t>());
			// auto& csb = allcomponents[hash];
			allcomponents[hash] = csb;
			csb->name = typeid(t).name();
			csb->h_update = typeid(&t::update) != typeid(&component::update);
			csb->h_lateUpdate = typeid(&t::lateUpdate) != typeid(&component::lateUpdate);
			if (((component *)&c)->_registerEngineComponent())
			{
				gameEngineComponents.insert(allcomponents[hash]);
			}
			else
			{
				gameComponents.insert(allcomponents[hash]);
			}
		}
		componentLock.unlock();
	}
	componentStorage<t> *compStorage = static_cast<componentStorage<t> *>(allcomponents[hash]);
	compStorage->lock.lock();
	typename deque_heap<t>::ref id = compStorage->data._new();
	compStorage->lock.unlock();
	*id = std::move(c);

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
