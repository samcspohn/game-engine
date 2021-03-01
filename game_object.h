#pragma once
#include "Component.h"
#include "Transform.h"
#include <map>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <list>
// #include "plf_list.h"
#include "fast_list.h"
#include "serialize.h"

using namespace std;

class _renderer;
class game_object_proto_;
// #define protoListRef typename std::list<game_object_proto_ *>::iterator
// #define protoList std::list<game_object_proto_ *>

class game_object;

extern tbb::concurrent_unordered_set<game_object *> toDestroyGameObjects;
// extern tbb::concurrent_unordered_set<component *> toStart;
// extern tbb::concurrent_unordered_set<component *> toDestroyComponents;
// extern tbb::concurrent_unordered_set<compItr *> componentCleanUp;

void registerProto(game_object_proto_ *p);
void deleteProtoRef(int id);

class game_object_proto_ : public assets::asset
{
public:
	// string name;
	game_object_proto_()
	{
		// ref = registerProto(this);
	}
	bool onEdit()
	{
	}
	string type()
	{
		return "GAME_OBJECT_TYPE";
	}
	void inspect()
	{
		int n{0};
		for (auto i = components.begin();
			 i != components.end();
			 i++)
		{
			ImGui::PushID(n);
			ImGui::SetNextItemOpen(true, ImGuiCond_Always);
			if (ImGui::TreeNode((to_string(n) + ComponentRegistry.components[i->second]->getName()).c_str()))
			{
				i->first->onEdit();
				ImGui::TreePop();
			}
			ImGui::PopID();
			n++;
		}

		if (ImGui::Button("add component"))
			ImGui::OpenPopup("add_component_context");
		if (ImGui::BeginPopup("add_component_context"))
		{
			for (auto &i : ComponentRegistry.meta)
			{
				if (ImGui::Selectable(i.first.c_str()))
				{
					i.second->addComponentProto(this);
				}
			}
			ImGui::EndPopup();
		}
	}
	map<component *, ull> components;
	// protoListRef ref;

	// game_object_proto(const game_object_proto& other){
	// 	for()
	// }
	template <class t>
	t *addComponent()
	{
		component *ret = new t();
		this->components.insert(std::pair<component *, ull>(ret, typeid(t).hash_code()));
		return (t *)ret;
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
	~game_object_proto_()
	{
		for (auto &i : components)
			delete i.first;
		deleteProtoRef(id);
	}
	friend class boost::serialization::access;

	template <class Archive>
	void serialize(Archive &ar, const unsigned int /* file_version */)
	{
		ar &boost::serialization::base_object<assets::asset>(*this) & components;
	}

	void serialize(OARCHIVE &ar, const unsigned int)
	{
		ar << boost::serialization::base_object<assets::asset>(*this);
		// ar << name;
		vector<string> archives;
		for (auto &c : components)
		{
			stringstream ss;
			OARCHIVE _ar(ss);
			_ar << c.second;
			_ar << c.first;
			archives.push_back(ss.str());
		}
		ar << archives;
	}
	void serialize(IARCHIVE &ar, const unsigned int)
	{
		ar >> boost::serialization::base_object<assets::asset>(*this);
		// ar >> name;
		vector<string> archives;
		ar >> archives;
		for (auto &s : archives)
		{
			ull type;
			component *c;
			stringstream ss{s};
			IARCHIVE _ar(ss);
			_ar >> type;
			try
			{
				_ar >> c;
			}
			catch (exception e)
			{
				// ComponentRegistry.getByType(type)->floatingComponent(c);
				cout << e.what() << endl;
			}
			components.emplace(pair<component *, ull>(c, type));
		}
	}
};

extern unordered_map<int, game_object_proto_ *> prototypeRegistry;

struct game_object_prototype
{
	int id;
	game_object_prototype();
	game_object_prototype(game_object_proto_ *p);
	SER_HELPER()
	{
		ar &id;
	}
};

void saveProto(OARCHIVE &oa);
void loadProto(IARCHIVE &ia);

class game_object : public inspectable
{
	mutex lock;
	mutex colLock;
	bool colliding = false;

	unordered_multimap<ull,std::pair<int,component*>> components;
	~game_object() { destroyed = false; };
	bool destroyed = false;
	//friend component;
	_renderer *renderer = 0;
	friend _renderer;
	friend void rebuildGameObject(componentStorageBase *, int);
	friend void save_level(const char *);
	friend void load_level(const char *);
	friend void loadTransforms(IARCHIVE &ia);
	friend class inspectorWindow;

public:
	void inspect()
	{
		transform2 t = this->transform;

		// position
		glm::vec3 pos = t.getPosition();
		glm::vec3 offset = pos;
		if (ImGui::DragFloat3("position", &pos.x))
		{
			t.translate(pos - offset);
			Transforms.updates[t.id].pos = true;
		}

		// rotation
		glm::vec3 angles = glm::eulerAngles(Transforms.rotations[t.id]);
		offset = angles;
		angles = glm::degrees(angles);
		if (ImGui::DragFloat3("rotation", &angles.x))
		{
			angles = glm::radians(angles);
			t.rotate(glm::vec3(1, 0, 0), angles.x - offset.x);
			t.rotate(glm::vec3(0, 1, 0), angles.y - offset.y);
			t.rotate(glm::vec3(0, 0, 1), angles.z - offset.z);
			Transforms.updates[t.id].rot = true;
		}

		// scale
		glm::vec3 sc = t.getScale();
		if (ImGui::DragFloat3("scale", &sc.x, 0.1))
		{
			t.setScale(sc);
			Transforms.updates[t.id].scl = true;
		}

		int n{0};
		for (auto i = t.gameObject()->components.begin();
			 i != t.gameObject()->components.end();
			 i++)
		{
			ImGui::PushID(n);
			ImGui::SetNextItemOpen(true, ImGuiCond_Always);
			if (ImGui::TreeNode((to_string(n) + ComponentRegistry.components[i->first]->getName()).c_str()))
			{
				ImGui::SameLine();
				if (ImGui::Button("x"))
				{
					this->_removeComponent(i->second.second);
				}
				else
					i->second.second->onEdit();
				ImGui::TreePop();
			}
			ImGui::PopID();
			n++;
		}

		if (ImGui::Button("add component"))
			ImGui::OpenPopup("add_component_context");
		if (ImGui::BeginPopup("add_component_context"))
		{
			for (auto &i : ComponentRegistry.meta)
			{
				if (ImGui::Selectable(i.first.c_str()))
				{
					i.second->addComponent(t->gameObject());
				}
			}
			ImGui::EndPopup();
		}
	}

	_renderer *getRenderer()
	{
		return (_renderer *)&*renderer;
	}
	template <typename t>
	t *getComponent()
	{
		ull hash = typeid(t).hash_code();
		for (auto &i : components)
			if (i.first == hash)
			{
				return (t *)i.second.second;
			}
		return 0;
	}
	template <typename t>
	vector<t *> getComponents()
	{
		vector<t *> ret;
		ull hash = typeid(t).hash_code();
		for (auto &i : components)
			if (i.first == hash)
			{
				ret.push_back((t *)i.second.second);
			}
		return ret;
	}

	void collide(game_object *go, glm::vec3 point, glm::vec3 normal)
	{
		// lock.lock();
		colLock.lock();
		colliding = true;
		if (destroyed)
		{
			colLock.unlock();
			return;
		}
		for (auto &i : components)
		{
			i.second.second->onCollision(go, point, normal);
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
		auto ci = addComponentToRegistry<t>();
		t *ret = (t *)ci.second;
		// ci.CompItr->goComponents = &this->components;
		components.insert(std::make_pair(typeid(t).hash_code(), ci));
		ret->transform = this->transform;
		ret->transform->setGameObject(this);
		ret->init();
		// toStart.emplace(ret);
		ret->onStart();
		// gameLock.unlock();

		return ret;
	}

	template <class t>
	t *_addComponent()
	{
		// gameLock.lock();
		auto ci = addComponentToRegistry<t>();
		t *ret = (t *)ci.second;
		// ci.CompItr->goComponents = &this->components;
		components.insert(std::make_pair(typeid(t).hash_code(), ci));
		ret->transform = this->transform;
		ret->transform->setGameObject(this);
		ret->init();

		// ret->onStart();
		// gameLock.unlock();

		return ret;
	}

	template <class t>
	t *addComponent(const t &c)
	{
		// gameLock.lock();
		auto ci = addComponentToRegistry(c);
		t *ret = (t *)ci.second;
		// ci.CompItr->goComponents = &this->components;
		components.insert(std::make_pair(typeid(t).hash_code(), ci));
		ret->transform = this->transform;
		ret->transform->setGameObject(this);
		ret->init();
		// toStart.emplace(ret);
		ret->onStart();
		// gameLock.unlock();

		return ret;
	}

	template <class t>
	t *dupComponent(const t &c)
	{
		// gameLock.lock();
		auto ci = addComponentToRegistry(c);
		t *ret = (t *)ci.second;
		// ci.CompItr->goComponents = &this->components;
		components.insert(std::make_pair(typeid(t).hash_code(), ci));
		ret->transform = this->transform;
		ret->transform->setGameObject(this);
		// gameLock.unlock();
		return ret;
	}
	template <class t>
	void removeComponent(t *c)
	{
		// gameLock.lock();
		ull hash = typeid(*c).hash_code();
		auto toR = components.find(hash);
		toR->second.second->deinit();
		// toDestroyComponents.emplace(toR->first);
		toR->second.second->onDestroy();
		// componentCleanUp.emplace(toR->second);
		COMPONENT_LIST(t)->_delete(toR->second.first);
		// toR->second->erase();
		components.erase(toR);
		// gameLock.unlock();
	}
	void removeComponent(component *c)
	{
		// gameLock.lock();
		ull hash = typeid(*c).hash_code();
		auto toR = components.find(hash);
		toR->second.second->deinit();
		// toDestroyComponents.emplace(toR->first);
		toR->second.second->onDestroy();
		// componentCleanUp.emplace(toR->second);
		ComponentRegistry.components.at(hash)->_delete(toR->second.first);
		
		components.erase(toR);
		// gameLock.unlock();
	}

	void _removeComponent(component *c)
	{
		// gameLock.lock();
		ull hash = typeid(*c).hash_code();
		auto toR = components.find(hash);
		toR->second.second->deinit();
		// toDestroyComponents.emplace(toR->first);
		// toR->second.second->onDestroy();
		// componentCleanUp.emplace(toR->second);
		ComponentRegistry.components.at(hash)->_delete(toR->second.first);
	}
	template <class t>
	void removeComponent()
	{
		ull hash = typeid(t).hash_code();
		auto toR = components.find(hash);
		toR->second.second->deinit();
		// toDestroyComponents.emplace(toR->first);
		toR->second.second->onDestroy();
		// componentCleanUp.emplace(toR->second);
		ComponentRegistry.components.at(hash)->_delete(toR->second.first);
	}

	void destroy()
	{
		toDestroyGameObjects.insert(this);
	}
	transform2 transform;

private:
	friend game_object *_instantiate(game_object_prototype &g);
	friend game_object *instantiate(game_object_prototype &g);
	friend game_object *_instantiate(game_object &g);
	friend game_object *instantiate(game_object &g);
	friend game_object *instantiate();
	friend game_object *_instantiate();
	friend void newGameObject(transform2 t);
	friend void run();
	friend void setRootGameObject(transform2 r);
	friend void stop_game(); 
	game_object(transform2 t) : lock()
	{
		// gameLock.lock();
		destroyed = false;
		this->transform = t;
		t->setGameObject(this);
		// gameLock.unlock();
	}
	game_object() : lock(){ };

	// game_object(game_object &g) : lock()
	// {
	// 	// gameLock.lock();
	// 	destroyed = false;
	// 	// this->transform = new Transform(*g.transform, this);
	// 	// g.transform->getParent()->Adopt(this->transform);

	// 	this->transform = Transforms._new(); // new Transform(this);
	// 	this->transform->init(g.transform, this);
	// 	g.transform.getParent().adopt(this->transform);
	// 	for (transform2 t : g.transform->getChildren())
	// 	{
	// 		new game_object(*t->gameObject(), transform);
	// 	}
	// 	// g.transform.getParent().adopt(this->transform);

	// 	for (auto &i : g.components)
	// 	{
	// 		i.first->_copy(this);
	// 	}
	// 	for (auto &i : this->components)
	// 	{
	// 		// toStart.emplace(i.first);
	// 		i.first->onStart();
	// 	}
	// 	// gameLock.unlock();
	// }
	static void startComponents(game_object &g)
	{
		for (auto &i : g.components)
		{
			i.second.second->onStart();
		}
		for (transform2 t : g.transform->getChildren())
		{
			startComponents(*t.gameObject());
		}
	}
	game_object(game_object &g, transform2 parent) : lock()
	{
		// gameLock.lock();
		destroyed = false;
		// this->transform = new Transform(*g.transform, this);
		// g.transform->getParent()->Adopt(this->transform);

		this->transform = Transforms._new(); // new Transform(this);
		this->transform->init(g.transform, this);
		parent.adopt(this->transform);
		for (transform2 t : g.transform->getChildren())
		{
			new game_object(*t->gameObject(), transform);
		}
		// g.transform.getParent().adopt(this->transform);

		for (auto &i : g.components)
		{
			i.second.second->_copy(this);
		}
		for (auto &i : this->components)
		{
			i.second.second->init();
		}

		// gameLock.unlock();
	}
	// game_object(const game_object_prototype &g) : lock()
	// {
	// 	game_object_proto_ &_g = *prototypeRegistry.at(g.id);
	// 	destroyed = false;
	// 	gameLock.lock();
	// 	this->transform = Transforms._new(); // new Transform(this);
	// 	this->transform->init(this);
	// 	gameLock.unlock();
	// 	for (auto &i : _g.components)
	// 	{
	// 		i.first->_copy(this);
	// 	}
	// 	for (auto &i : this->components)
	// 	{
	// 		// toStart.emplace(i.first);
	// 		i.first->onStart();
	// 	}
	// }
	void _destroy()
	{
		lock.lock();
		if (this->colliding)
		{
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
			i.second.second->deinit();
			i.second.second->onDestroy();
		}

		for(auto &c : components)
		{
			// c.second->erase();
			ComponentRegistry.components.at(c.first)->_delete(c.second.first);
			// components.erase(components.begin());
		}
		while (transform->getChildren().size() > 0)
		{
			transform->getChildren().front()->gameObject()->_destroy();
		}
		transform->_destroy();
		lock.unlock();
		// this->_destroy();
		delete this;
	}
};

game_object *_instantiate(game_object &g);
game_object *instantiate(game_object &g);
game_object *instantiate();
game_object *_instantiate();
game_object *_instantiate(game_object_prototype &g);
game_object *instantiate(game_object_prototype &g);