#pragma once
#include <vector>
#include <map>
#include <list>
#include <typeinfo>
#include "Transform.h"
#include "concurrency.h"
#include "plf_list.h"
#include <stdexcept>
#include "listThing2.h"
#include "fast_list.h"
#include "array_heap.h"
#include <boost/thread/barrier.hpp>
#define ull unsigned long long
class game_object;
class Transform;
class component;
bool compareTransform(Transform *t1, Transform *t2);

struct compItr
{
	ull hash;
	map<component*,compItr*>* goComponents;
	virtual void erase(){};
	virtual component *getComponent(){};
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
		goComponents->erase(getComponent());
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
	component* compPtr;
	compItr *CompItr;
};

class componentStorageBase
{
public:
	string name;

	virtual void update(){};
	virtual void update(int index, int size) {}
	virtual void lateUpdate(int index, int size) {}

	virtual int size(){};
	virtual void sort(){};
};

class component
{
public:
	int threadID;
	virtual void onStart() {}
	virtual void onDestroy() {}

	virtual bool _registerEngineComponent() { return false; };
	virtual void onCollision(game_object *go){};
	virtual void _update(int index, unsigned int _start, unsigned int _end){};
	virtual void _lateUpdate(int index, unsigned int _start, unsigned int _end){};
	virtual void _copy(game_object *go) = 0;
	Transform *transform;
	int getThreadID()
	{
		return threadID;
	}
	ull getHash()
	{
		return typeid(*this).hash_code();
	}
};

template <typename t>
class componentStorage : public componentStorageBase
{
public:
	deque_heap<t> data;

	componentStorage()
	{
		//		data = listThing2<t>();
	}
	int size()
	{
		return data.size();
	}

	// void sort()
	// {
	// 	float unsorted = 0;
	// 	while (unsorted > 0)
	// 	{
	// 		unsorted = 0;
	// 		for (unsigned int i = 0; i < data.data.size() - 1; ++i)
	// 		{
	// 			if (compareTransform(((component *)&data.data[i])->transform, ((component *)&data.data[i + 1])->transform))
	// 			{
	// 				data.swap(i, i + 1);
	// 				unsorted += 1;
	// 			}
	// 		}
	// 	}
	// }
	void update(){
		((component *)&(data.data.front()))->_update(0, 0, data.size());
	}
	void update(int index, int size)
	{
		unsigned int _start = (unsigned int)((float)size / (float)concurrency::numThreads * (float)index);
		unsigned int _end = (unsigned int)((float)size / (float)concurrency::numThreads * (float)(index + 1));
		if (index == concurrency::numThreads - 1)
			_end = size;
		((component *)&(data.data.front()))->_update(index, _start, _end);
	}
	void lateUpdate(int index, int size)
	{
		unsigned int _start = (unsigned int)((float)size / (float)concurrency::numThreads * (float)index);
		unsigned int _end = (unsigned int)((float)size / (float)concurrency::numThreads * (float)(index + 1));
		if (index == concurrency::numThreads - 1)
			_end = size;
		((component *)&(data.data.front()))->_lateUpdate(index, _start, _end);
	}
};

std::map<ull, componentStorageBase *> allcomponents;
std::set<componentStorageBase *> gameEngineComponents;
std::set<componentStorageBase *> gameComponents;
std::mutex componentLock;
template <typename t>
inline compInfo<t> addComponentToAll(const t &c)
{

	componentLock.lock();
	ull hash = typeid(t).hash_code();

	if (allcomponents.find(hash) == allcomponents.end())
	{
		allcomponents[hash] = (componentStorageBase *)(new componentStorage<t>());
		allcomponents[hash]->name = typeid(t).name();
		if (((component *)&c)->_registerEngineComponent())
		{
			gameEngineComponents.insert(allcomponents[hash]);
		}
		else
		{
			gameComponents.insert(allcomponents[hash]);
		}
	}
	componentStorage<t> *compStorage = static_cast<componentStorage<t> *>(allcomponents[hash]);
	typename deque_heap<t>::ref id = compStorage->data._new();
	*id = std::move(c);

	compInfo<t> ret;
	ret.compPtr = &(*id);
	ret.CompItr = new compItr_<t>(id, &compStorage->data);
	ret.CompItr->hash = hash;
	componentLock.unlock();
	return ret;
}

void ComponentsUpdate(componentStorageBase *csbase, int i, int size)
{
	csbase->update(i, size);
}

void ComponentsLateUpdate(componentStorageBase *csbase, int i, int size)
{
	csbase->lateUpdate(i, size);
}

#define COMPONENT_LIST(x) static_cast<componentStorage<x> *>(allcomponents[typeid(x).hash_code()])

#define COPY(component_type)                     \
	void _copy(game_object *go)                  \
	{                                            \
		go->dupComponent(component_type(*this)); \
	}
//#define UPDATE(component_type, update_function) void _update(int index, unsigned int _start, unsigned int _end){ \
//    listThing2<component_type>::node* i = COMPONENT_LIST(component_type)->data[_start];\
//    listThing2<component_type>::node* end;\
//    bool isEnd = _end >= COMPONENT_LIST(component_type)->data.accessor.size();\
//    if(isEnd)\
//        end = COMPONENT_LIST(component_type)->data[_end - 1];\
//    else\
//        end = COMPONENT_LIST(component_type)->data[_end];\
//	for (i; i != end; i = i->next) { i->value.threadID = index; i->value.update_function();  } \
//    if(isEnd){ end->value.threadID = index; end->value.update_function(); }\
// }
#define UPDATE(component_type, update_function)                                                         \
	void _update(int index, unsigned int _start, unsigned int _end)                                     \
	{                                                                                                   \
		deque<component_type>::iterator i = COMPONENT_LIST(component_type)->data.data.begin() + _start; \
		deque<component_type>::iterator end = COMPONENT_LIST(component_type)->data.data.begin() + _end; \
		deque<bool>::iterator val = COMPONENT_LIST(component_type)->data.valid.begin() + _start;        \
		for (i; i != end; ++i, ++val)                                                                   \
		{                                                                                               \
			if (*val)                                                                                   \
			{                                                                                           \
				(*i).threadID = index;                                                                  \
				(*i).update_function();                                                                 \
			}                                                                                           \
		}                                                                                               \
	}
#define LATE_UPDATE(component_type, late_update_function)                                               \
	void _lateUpdate(int index, unsigned int _start, unsigned int _end)                                 \
	{                                                                                                   \
		deque<component_type>::iterator i = COMPONENT_LIST(component_type)->data.data.begin() + _start; \
		deque<component_type>::iterator end = COMPONENT_LIST(component_type)->data.data.begin() + _end; \
		deque<bool>::iterator val = COMPONENT_LIST(component_type)->data.valid.begin() + _start;        \
		for (i; i != end; ++i, ++val)                                                                          \
		{                                                                                               \
			if (*val)                                                                                   \
			{                                                                                           \
				(*i).threadID = index;                                                                  \
				(*i).late_update_function();                                                            \
			}                                                                                           \
		}                                                                                               \
	}

//#define UPDATE(component_type, update_function) void _update(int index, unsigned int _start, unsigned int _end){ \
//    vector<component_type>& d = COMPONENT_LIST(component_type)->data.data;\
//    for (int i = index; i < _end; i += concurrency::numThreads) { d[i].threadID = index; d[i].update_function();  }\
//}