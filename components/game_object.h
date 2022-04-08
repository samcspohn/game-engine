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

struct placeholder : public component{
    YAML::Node data;
    void ser_edit(ser_mode x, YAML::Node &n);
};

using namespace std;

class _renderer;
class game_object_proto_;
class game_object;


extern std::mutex toDestroym;
extern std::vector<game_object *> toDestroyGameObjects;

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
		return false;
	}
	string type()
	{
		return "GAME_OBJECT_PROTO_TYPE";
	}
	void inspect()
	{
		int n{0};
		for (auto i = components.begin(); i != components.end();)
		{
			ImGui::PushID(n);
			ImGui::SetNextItemOpen(true, ImGuiCond_Always);
			YAML::Node none;
			if (ImGui::TreeNode((to_string(n) + ComponentRegistry.registry(i->second)->getName()).c_str()))
			{
				ImGui::SameLine();
				if (ImGui::Button("x"))
				{
					i = components.erase(i);
				}
				else
				{
					i->first->ser_edit(ser_mode::edit_mode, none);
					i++;
				}
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
		// for (auto &i : components)
		// 	delete i.first;
		// deleteProtoRef(id);
	}
};

namespace YAML
{
	template <>
	struct convert<game_object_proto_>
	{
		static Node encode(const game_object_proto_ &rhs);

		static bool decode(const Node &node, game_object_proto_ &rhs);
	};
}

struct gameObjectProtoManager : public assets::assetManager<game_object_proto_>
{
	void _new();
};
extern gameObjectProtoManager game_object_proto_manager;

struct game_object_prototype : public assets::asset_instance<game_object_proto_>
{
	game_object_prototype();
	game_object_prototype(game_object_proto_ *p);
	game_object_proto_ *meta() const;
};
void renderEdit(const char *name, game_object_prototype &g);

namespace YAML
{

	template <>
	struct convert<game_object_prototype>
	{
		static Node encode(const game_object_prototype &rhs)
		{
			Node node;
			node = rhs.id;
			return node;
		}

		static bool decode(const Node &node, game_object_prototype &rhs)
		{
			rhs.id = node.as<int>();
			return true;
		}
	};
}

extern unordered_map<int, glm::vec3> transform_euler_angles;

class game_object : public inspectable
{
	// mutex lock;
	// mutex colLock;
	bool colliding = false;

	multimap<size_t, int> components = {};
	bool destroyed = false;
	// friend component;
	friend _renderer;
	friend void rebuildGameObject(componentStorageBase *, int);
	friend void save_level(string);
	friend void load_level(string);
	friend int main(int argc, char **argv);
	friend class inspectorWindow;
	friend class particle_emitter;
	static component *_getComponent(pair<size_t, int> i)
	{
		return ComponentRegistry.registry(i.first)->get(i.second);
	}

public:
	static void encode(YAML::Node &node, game_object *g);
	static void decode(YAML::Node &node, int, list<function<void()>> * = 0);

	void inspect()
	{

		vector<char> text(transform.name().length() + 200, 0);
		// cout << "s: " << s.size() << ": " << s.length() << endl;
		sprintf(text.data(), transform.name().c_str());
		if (ImGui::InputText("name", text.data(), text.size()))
		{
			transform.name() = string{text.data()};
		}

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
		if (transform_euler_angles.find(t.id) == transform_euler_angles.end())
		{
			glm::vec3 angles = glm::eulerAngles(Transforms.rotations[t.id]);
			transform_euler_angles.emplace(t.id, glm::degrees(angles));
		}
		if (ImGui::DragFloat3("rotation", &transform_euler_angles.at(t.id).x))
		{
			// glm::quat _q;
			glm::vec3 angles = glm::radians(transform_euler_angles.at(t.id));
			t.setRotation(angles);
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
			if (ImGui::TreeNode(ComponentRegistry.registry(i->first)->getName().c_str()))
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
				return static_cast<t *>(ComponentRegistry.registry(hash)->get(i.second));
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
				ret.push_back(static_cast<t *>(ComponentRegistry.registry(hash)->get(i.second)));
			}
		return ret;
	}

	void collide(collision &_collision)
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
			_getComponent(i)->onCollision(_collision);
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
		t *ci = static_cast<t *>(ComponentRegistry.registry(hash)->get(i));
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
		t *ci = static_cast<t *>(ComponentRegistry.registry(hash)->get(i));
		ci->transform = this->transform;
		ci->init(i);
		return ci;
	}

	template <class t>
	t *addComponent(const t &c)
	{
		t *ci = _addComponent(c);
		ci->onStart();
		return ci;
	}

	// template <class t>
	// t *dupComponent(const t &c)
	// {
	// 	size_t hash = typeid(t).hash_code();
	// 	int i = addComponentToRegistry(c);
	// 	components.emplace(hash, i);
	// 	t *ci = ComponentRegistry.registry(hash)->get(i);
	// 	ci->transform = this->transform;
	// 	return &(*ci);
	// }
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
	friend void newFile();

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

template <typename t>
void componentStorage<t>::addComponent(game_object *g)
{
	g->_addComponent<t>();
}
template <typename t>
void componentStorage<t>::addComponentProto(game_object_proto_ *g)
{
	g->addComponent<t>();
}
extern STORAGE<game_object> game_object_cache;
game_object *_instantiate(game_object &g);
game_object *instantiate(game_object &g);
game_object *instantiate();
game_object *_instantiate();
game_object *_instantiate(game_object_prototype &g);
game_object *instantiate(game_object_prototype &g);