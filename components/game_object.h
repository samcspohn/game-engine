#pragma once
#include "Component.h"
// #include "Transform.h"
#include <map>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <list>
// #include "plf_list.h"
// #include "fast_list.h"
#include "_serialize.h"
#include "editor.h"
#include <tbb/tbb.h>
#include "imgui/imgui.h"
#include <tbb/concurrent_vector.h>
using namespace std;

class _renderer;
class game_object_proto_;
// #define protoListRef typename std::list<game_object_proto_ *>::iterator
// #define protoList std::list<game_object_proto_ *>

class game_object;

extern std::mutex toDestroym;
// extern std::deque<game_object *> toDestroyGameObjects;
extern std::vector<game_object *> toDestroyGameObjects;
// extern tbb::concurrent_vector<game_object *> toDestroyGameObjects;

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
	// void inspect()
	// {
	// 	int n{0};
	// 	for (auto i = components.begin();
	// 		 i != components.end();
	// 		 i++)
	// 	{
	// 		ImGui::PushID(n);
	// 		ImGui::SetNextItemOpen(true, ImGuiCond_Always);
	// 		if (ImGui::TreeNode((to_string(n) + ComponentRegistry.components[i->second]->getName()).c_str()))
	// 		{
	// 			i->first->onEdit();
	// 			ImGui::TreePop();
	// 		}
	// 		ImGui::PopID();
	// 		n++;
	// 	}

	// 	if (ImGui::Button("add component"))
	// 		ImGui::OpenPopup("add_component_context");
	// 	if (ImGui::BeginPopup("add_component_context"))
	// 	{
	// 		for (auto &i : ComponentRegistry.meta)
	// 		{
	// 			if (ImGui::Selectable(i.first.c_str()))
	// 			{
	// 				i.second->addComponentProto(this);
	// 			}
	// 		}
	// 		ImGui::EndPopup();
	// 	}
	// }
	map<component *, size_t> components;
	// protoListRef ref;

	// game_object_proto(const game_object_proto& other){
	// 	for()
	// }
	template <class t>
	t *addComponent()
	{
		component *ret = new t();
		this->components.emplace(ret, typeid(t).hash_code());
		return (t *)ret;
	}

	template <typename t>
	t *getComponent()
	{
		size_t hash = typeid(t).hash_code();
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
			size_t type;
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
			components.emplace(c, type);
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
	// mutex lock;
	// mutex colLock;
	bool colliding = false;

	multimap<size_t, int> components = {};
	bool destroyed = false;
	//friend component;
	friend _renderer;
	friend void rebuildGameObject(componentStorageBase *, int);
	friend void save_level(const char *);
	friend void load_level(const char *);
	friend void loadTransforms(IARCHIVE &ia);
	friend int main(int argc, char **argv);
	friend class inspectorWindow;
	friend class particle_emitter;
	static component *_getComponent(pair<size_t, int> i)
	{
		return ComponentRegistry.registry(i.first)->get(i.second);
	}

public:
	// ~game_object() { destroyed = false; };

	static void serialize(OARCHIVE& ar, game_object* g);
	static void deserialize(IARCHIVE& ar, map<int,int>& transform_map);

	static void encode(YAML::Node& node, game_object* g);
	static void decode(YAML::Node& node, int);

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

		YAML::Node none;
		int n{0};
		for (auto i = t.gameObject()->components.begin(); i != t.gameObject()->components.end(); i++)
		{
			ImGui::PushID(n);
			ImGui::SetNextItemOpen(true, ImGuiCond_Always);
			if (ImGui::TreeNode((to_string(n) + ComponentRegistry.registry(i->first)->getName()).c_str()))
			{
				ImGui::SameLine();
				if (ImGui::Button("x"))
				{
					this->_removeComponent(_getComponent(*i));
				}
				else
					_getComponent(*i)->ser_edit(ser_mode::edit_mode, none);
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
					// ImGui::EndPopup();
					// ImGui::CloseCurrentPopup();
					break;
				}
			}
			ImGui::EndPopup();
		}
	}

	template <typename t>
	t *getComponent()
	{
		size_t hash = typeid(t).hash_code();
		for (auto &i : components)
			if (i.first == hash)
			{
				return (t *)ComponentRegistry.registry(hash)->get(i.second);
				// return (t *)i.first;
			}
		return 0;
	}
	template <typename t>
	vector<t *> getComponents()
	{
		vector<t *> ret;
		size_t hash = typeid(t).hash_code();
		for (auto &i : components)
			if (i.first == hash)
			{
				ret.push_back((t *)ComponentRegistry.registry(hash)->get(i.second));
			}
		return ret;
	}

	void collide(game_object *go, glm::vec3 point, glm::vec3 normal)
	{
		// lock.lock();
		// colLock.lock();
		colliding = true;
		if (destroyed)
		{
			// colLock.unlock();
			return;
		}
		for (auto &i : components)
		{
			_getComponent(i)->onCollision(go, point, normal);
			// i.first->onCollision(go, point, normal);
		}
		colliding = false;
		// colLock.unlock();
		// if(this->destroyed){
		// 	this->destroy();
		// }
		// lock.unlock(); 
	}

	template <class t>
	t *addComponent()
	{
		t *ret = _addComponent<t>();
		ret->onStart();
		return ret;
	}

	template <class t>
	t *_addComponent()
	{
		size_t hash = typeid(t).hash_code();
		int i = addComponentToRegistry<t>();
		components.emplace(hash, i);
		t* ci = ComponentRegistry.registry(hash)->get(i);
		ci->transform = this->transform;
		ci->init(i);
		return &(*ci);
	}

	template <class t>
	t *_addComponent(const t &c)
	{
		size_t hash = typeid(t).hash_code();
		int i = addComponentToRegistry(c);
		components.emplace(hash, i);
		t* ci = ComponentRegistry.registry(hash)->get(i);
		ci->transform = this->transform;
		ci->init(i);
		return ci;
	}

	template <class t>
	t *addComponent(const t &c)
	{
		t* ci = _addComponent(c);
		ci->onStart();
		return ci;
	}

	template <class t>
	t *dupComponent(const t &c)
	{
		size_t hash = typeid(t).hash_code();
		int i = addComponentToRegistry(c);
		components.emplace(hash, i);
		t* ci = ComponentRegistry.registry(hash)->get(i);
		ci->transform = this->transform;
		return &(*ci);
	}
	template <class t>
	void removeComponent(t *c)
	{
		size_t hash = typeid(t).hash_code();
		auto range = components.equal_range(hash);
		for (auto &i = range.first; i != range.second; i++)
		{
			if (_getComponent(*i) == c)
			{
				c->onDestroy();
				c->deinit(i->second);
				components.erase(i);
				return;
			}
		}
	}
	void removeComponent(component *c)
	{
		for (auto i = components.begin(); i != components.end(); i++)
		{
			if (_getComponent({i->first, i->second}) == c)
			{
				c->onDestroy();
				c->deinit(i->second);
				components.erase(i);
				ComponentRegistry.registry(i->first)->erase(i->second);
				return;
			}
		}
	}

	void _removeComponent(component *c)
	{
		for (auto i = components.begin(); i != components.end(); i++)
		{
			if (_getComponent({i->first, i->second}) == c)
			{
				c->onDestroy();
				c->deinit(i->second);
				components.erase(i);
				ComponentRegistry.registry(i->first)->erase(i->second);
				return;
			}
		}
	}
	template <class t>
	void removeComponent()
	{
		size_t hash = typeid(t).hash_code();
		auto range = components.equal_range(hash);
		for (auto &i = range.first; i != range.second; i++)
		{
			t *c = _getComponent(*i);
			c->onDestroy();
			c->deinit(i->second);
			components.erase(i);
			return;
		}
	}

	void destroy()
	{
		std::lock_guard<std::mutex> lk(toDestroym);
		toDestroyGameObjects.push_back(this);
	}
	transform2 transform;
	// game_object() = default;

private:
	friend void _child_instatiate(game_object &g, transform2 parent);
	friend game_object *_instantiate(game_object_prototype &g);
	friend game_object *instantiate(game_object_prototype &g);
	friend game_object *_instantiate(game_object &g);
	friend game_object *instantiate(game_object &g);
	friend game_object *instantiate();
	friend game_object *_instantiate();
	// friend class STORAGE<game_object>;
	friend void newGameObject(transform2 t);
	friend void run();
	friend void setRootGameObject(transform2 r);
	friend void stop_game();

	static void startComponents(game_object &g)
	{
		for (auto &i : g.components)
		{
			game_object::_getComponent(i)->onStart();
		}
		for (transform2 t : g.transform->getChildren())
		{
			startComponents(*t.gameObject());
		}
	}
	void _destroy();
};

extern STORAGE<game_object> game_object_cache;
game_object *_instantiate(game_object &g);
game_object *instantiate(game_object &g);
game_object *instantiate();
game_object *_instantiate();
game_object *_instantiate(game_object_prototype &g);
game_object *instantiate(game_object_prototype &g);