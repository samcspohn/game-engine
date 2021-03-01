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
// #include "game_object.h"
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

	static bool _registerEngineComponent();
	virtual void onCollision(game_object *go, glm::vec3 point, glm::vec3 normal);
	virtual void update();
	virtual void lateUpdate();
	virtual void init();
	virtual void deinit();

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

// struct compItr
// {
// 	ull hash;
// 	// map<component *, compItr *> *goComponents;
// 	virtual void erase();
// 	virtual component *getComponent();
// };

// template <typename t>
// struct compItr_ : public compItr
// {
// 	typename deque_heap<t>::ref id;
// 	deque_heap<t> *l;
// 	void erase()
// 	{
// 		//	    delete *id;
// 		// (&(id.data()))->onDestroy();
// 		// goComponents->erase(getComponent());
// 		l->_delete(id);
// 		delete this;
// 	}
// 	typename deque_heap<t>::ref get()
// 	{
// 		return id;
// 	}
// 	component *getComponent()
// 	{
// 		return (component *)&(*id);
// 	}
// 	compItr_(typename deque_heap<t>::ref _id, deque_heap<t> *_l) : id(_id), l(_l) {}
// 	compItr_() {}
// };

// struct compInfo
// {
// 	component *compPtr;
// 	compItr *CompItr;
// };


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
	// virtual void sort(){};
	virtual void _delete(int i) {};
	virtual std::pair<int,component*> getInfo(int i) { return std::pair<int,component*>(0,0); };
	virtual void clear() {}
	virtual void s() {}
	// virtual string ser(){};

	friend class boost::serialization::access;
	template <class Archive>
	void serialize(Archive &ar, const unsigned int /* file_version */)
	{
		ar &name &h_update &h_lateUpdate;
	}
};
void rebuildGameObject(componentStorageBase *base, int i);

// std::ostream & operator<<(std::ostream &os, const componentStorageBase &base)
// {
//     return os << base.name << base.h_update << base.h_lateUpdate << base.ser();
// }
REGISTER_BASE(componentStorageBase)

template <typename t>
class componentStorage : public componentStorageBase
{
public:
	// mutex m;
	// deque_heap<t> data;
	tbb::concurrent_unordered_map<int,t> _data;
	tbb::concurrent_priority_queue<int, std::greater<int>> avail;

	// componentStorage() : lock()
	// {
	// }

	void s(){
		tbb::parallel_for_each(_data.range(),[&](auto& i){
			rebuildGameObject(this,i.first);
		});
			// for (int j = 0; j < i.second->size(); j++)
			// {
			// 	// if (i.second->getv(j))
			// 	// {
					// rebuildGameObject(i.second, j);
			// 	// 	i.second->get(j)->init();
			// 	// }
			// }
	}
	unsigned int active()
	{
		return _data.size();

		// return data.active;
	}
	int size()
	{
		return _data.size();
	}

	t *get(int i)
	{

		if(_data.find(i) != _data.end()){
			return &_data.at(i);
		}
		return 0;
		// if (i >= _data.size())
		// 	return 0;
		// else
		// 	return (t *)&(_data[i]);
	}
	bool getv(int i)
	{
		if(_data.find(i) != _data.end()){
			return true;
		}
		return false;
		// if (i >= _data.size())
		// 	return 0;
		// else
		// 	return data.valid[i];
	}
	std::pair<int,component*> getInfo(int i) 
	{
		// compInfo ret;
		// ret.compPtr = &(*id);
		// ret.CompItr = new compItr_<t>(id, &data);
		// ret.CompItr->hash = typeid(t).hash_code();
		return std::pair<int,component*>(i,&_data[i]);
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
			// _parallel_for(data, [&](int i) {
			// 	if (data.valid[i])
			// 	{
			// 		data.data[i].update();
			// 	}
			// });
			tbb::parallel_for_each(_data.range(),[](auto& i){i.second.update();});
		}
		else
		{
			std::for_each(_data.begin(),_data.end(),[](auto& i){i.second.update();});
			// int size = data.size();
			// for (int i = 0; i < size; i++)
			// {
			// 	if (data.valid[i])
			// 	{
			// 		data.data[i].update();
			// 	}
			// }
		}
		update_t = update_timer.stop();
	}
	void lateUpdate()
	{
		update_timer.start();
		if (lateupdate_t > 0.1f)
		{
			// _parallel_for(data, [&](int i) {
			// 	if (data.valid[i])
			// 	{
			// 		data.data[i].lateUpdate();
			// 	}
			// });
			tbb::parallel_for_each(_data.range(),[](auto& i){i.second.lateUpdate();});

		}
		else
		{
			// int size = data.size();
			// for (int i = 0; i < size; i++)
			// {
			// 	if (data.valid[i])
			// 	{
			// 		data.data[i].lateUpdate();
			// 	}
			// }
			std::for_each(_data.begin(),_data.end(),[](auto& i){i.second.lateUpdate();});
		}
		lateupdate_t = update_timer.stop();
	}
	void clear()
	{
		_data.clear();
		avail.clear();
	}
	template<typename... types>
	std::pair<int,component*> _new(types... args){
		int i;
		
		if(!avail.try_pop(i)){
			lock.lock();
			i = _data.size();
			_data.emplace(i,t(args...));
			lock.unlock();
		}else{	
			_data.emplace(i,t(args...));
		}
		return std::pair<int,t*>(i,&_data[i]);
	}
	void _delete(int i){
		lock.lock();
		avail.push(i);
		_data.unsafe_erase(i);
		lock.unlock();
	}
	friend class boost::serialization::access;

	template <class Archive>
	void serialize(Archive &ar, const unsigned int /* file_version */) {}
	void serialize(OARCHIVE &ar, const unsigned int)
	{
		string s;
		unordered_map<int,int> transforms;
		// unordered_map<int,t> data(_data.begin(),_data.end());
		
		vector<int> av;
		int d;
		while(avail.try_pop(d)){
			av.push_back(d);
		}
		for(int i : av){
			avail.push(i);
		}


		for (auto &i : _data)
		{
			transforms.emplace(i.first, i.second.transform.id);
		}
		{
			stringstream ss;
			OARCHIVE _ar(ss);
			_ar << av;
			_ar << _data.size();
			for(auto& i : _data){
				_ar << i.first;
				_ar << i.second;
			}
			s = ss.str();
		}
		ar << boost::serialization::base_object<componentStorageBase>(*this) << transforms << s;
	}
	void serialize(IARCHIVE &ar, const unsigned int)
	{
		string s;
		unordered_map<int,int> transforms;
		// unordered_map<int,t> data;
		vector<int> av;

		ar >> boost::serialization::base_object<componentStorageBase>(*this) >> transforms >> s;
		{
			stringstream ss{s};
			try
			{
				IARCHIVE _ar(ss);
				_ar >> av;
				avail = tbb::concurrent_priority_queue<int, std::greater<int>> (av.begin(),av.end());
				int size;
				_ar >> size;
				for(int i = 0; i < size; ++i){
					int id;
					t type;
					_ar >> id;
					_ar >> type;
					_data.emplace(std::move(pair<int,t>(id,std::move(type))));
				}
				// _data.insert(data.begin(),data.end());
			}
			catch (exception e)
			{
				// data.data.resize(transforms.size());
				cout << e.what() << endl;
			}
			// s = ss.str();
		}
		for (auto &i : transforms)
		{
			_data[i.first].transform.id = i.second;
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
	// virtual void removeComponent(game_object *g);
	// virtual void removeComponentProto(game_object_proto_ *g);
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

	void clear()
	{
		for (auto &i : components)
		{
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
			// if (t::_registerEngineComponent())
			// 	ComponentRegistry.gameEngineComponents.insert(pair(hash, ComponentRegistry.components[hash]));
			// else
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
inline std::pair<int,component*> addComponentToRegistry(const t &c)
{
	componentStorage<t> *compStorage = GetStorage<t>();
	return compStorage->_new(c);
	// typename deque_heap<t>::ref id = compStorage->data._new(c);
	// return compStorage->getInfo(id);
}

template <typename t>
inline std::pair<int,component*> addComponentToRegistry()
{
	componentStorage<t> *compStorage = GetStorage<t>();
	return compStorage->_new();
	// typename deque_heap<t>::ref id = compStorage->data._new();
	// return compStorage->getInfo(id);
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
			g->_addComponent<comp>();                     \
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
