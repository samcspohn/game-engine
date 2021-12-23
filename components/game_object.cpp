#include "game_object.h"

void placeholder::ser_edit(ser_mode x, YAML::Node &n)
{
	switch (x)
	{
	case ser_mode::edit_mode:
	{

		ImGui::Text("place holder");
		if(ImGui::Button("replace"))
			ImGui::OpenPopup("replace_component_context");
		if (ImGui::BeginPopup("replace_component_context"))
		{
			for (auto &i : ComponentRegistry.meta)
			{
				if (ImGui::Selectable(i.first.c_str()))
				{
					i.second->addComponent(this->transform->gameObject());
				}
			}
			ImGui::EndPopup();
		}
		stringstream ss;
		ss << this->data;
		// ImGui::Text("place holder 2");
		ImGui::Text(ss.str().c_str());
	}
	break;
	case ser_mode::read_mode:
		this->data = n["data"].as<YAML::Node>();
		break;
	case ser_mode::write_mode:
		n["data"] = this->data;
		break;

	default:
		break;
	}
}

REGISTER_COMPONENT(placeholder);

std::mutex toDestroym;
// std::deque<game_object *> toDestroyGameObjects;
std::vector<game_object *> toDestroyGameObjects;
// tbb::concurrent_vector<game_object *> toDestroyGameObjects;
// std::unordered_map<int, shared_ptr<game_object_proto_>> prototypeRegistry;

STORAGE<game_object> game_object_cache;
gameObjectProtoManager game_object_proto_manager;

// void encodePrototypes(YAML::Node &node)
// {
// 	YAML::Node prototypeRegistry_node;
// 	for (auto &i : prototypeRegistry)
// 	{
// 		prototypeRegistry_node.force_insert(i.first, *i.second);
// 	}
// 	node["prototype_registry"] = prototypeRegistry_node;
// }

// void decodePrototypes(YAML::Node &node)
// {
// 	for(auto &p : prototypeRegistry){
// 		for (auto &i : p.second->components)
// 			delete i.first;
// 	}
// 	prototypeRegistry.clear();

// 	YAML::Node prototypeRegistry_node = node["prototype_registry"];
// 	for (YAML::const_iterator i = prototypeRegistry_node.begin(); i != prototypeRegistry_node.end(); ++i)
// 	{
// 		prototypeRegistry[i->first.as<int>()] = make_shared<game_object_proto_>(i->second.as<game_object_proto_>());
// 	}
// }

void game_object::encode(YAML::Node &game_object_node, game_object *g)
{
	game_object_node["name"] = string(g->transform->name());
	// transform
	YAML::Node transform_node;
	transform_node["position"] = g->transform->getPosition();
	transform_node["rotation"] = g->transform->getRotation();
	transform_node["scale"] = g->transform->getScale();
	transform_node["id"] = g->transform.id;
	game_object_node["transform"] = transform_node;

	// components
	YAML::Node components_node;
	for (auto &i : g->components)
	{
		YAML::Node component_node;
		component_node["id"] = i.first;
		YAML::Node component_val_node;
		ComponentRegistry.registry(i.first)->encode(component_val_node, i.second);
		component_node["value"] = component_val_node;
		components_node.push_back(component_node);
	}
	game_object_node["components"] = components_node;

	// children
	YAML::Node transform_children;
	transform_node["children"] = transform_children;
	for (auto &i : g->transform->getChildren())
	{
		YAML::Node child_game_object_node;
		game_object::encode(child_game_object_node, i->gameObject());
		transform_children.push_back(child_game_object_node);
	}
}
void game_object::decode(YAML::Node &game_object_node, int parent_id, list<function<void()>> *defered_component_init)
{
	bool root_decode = false;
	if (defered_component_init == 0)
	{
		root_decode = true;
		defered_component_init = new list<function<void()>>();
		transform_map.clear();
	}
	int ref = game_object_cache._new();
	game_object *g = &game_object_cache.get(ref);

	g->transform = Transforms._new();

	try
	{
		g->transform->name() = game_object_node["name"].as<string>();
	}
	catch (...)
	{
	}
	g->transform->setPosition(game_object_node["transform"]["position"].as<glm::vec3>());
	g->transform->setRotation(game_object_node["transform"]["rotation"].as<glm::quat>());
	g->transform->setScale(game_object_node["transform"]["scale"].as<glm::vec3>());

	transform_map[game_object_node["transform"]["id"].as<int>()] = g->transform;

	Transforms.meta[g->transform.id].gameObject = ref;
	if (parent_id != -1)
		transform2(parent_id).adopt(g->transform);

	YAML::Node components_node = game_object_node["components"];

	defered_component_init->push_back([=]()
									  {
										  for (int i = 0; i < components_node.size(); i++)
										  {
											  YAML::Node component_node = components_node[i];
											  size_t hash = component_node["id"].as<size_t>();
											  YAML::Node component_val_node = component_node["value"];

											  if (hash == typeid(placeholder).hash_code() && ComponentRegistry.registry(component_val_node["data"]["id"].as<size_t>()) != 0)
											  {
												  hash = component_val_node["data"]["id"].as<size_t>();
												  component_val_node = component_val_node["data"]["value"];
											  } // if registry does not contain component placeholder is referencing then placeholder is de-serialized

											  if (ComponentRegistry.registry(hash))
											  {

												  int id = ComponentRegistry.registry(hash)->decode(component_val_node);
												  g->components.emplace(hash, id);
												  _getComponent(pair<size_t, int>(hash, id))->transform = g->transform;
											  }
											  else
											  {
												  //   int id = ComponentRegistry.registry(typeid(placeholder).hash_code());
												  placeholder *p = g->_addComponent<placeholder>();
												  p->data = component_node;
											  }
										  }
									  });

	YAML::Node transform_children = game_object_node["transform"]["children"];
	for (int i = 0; i < transform_children.size(); i++)
	{
		YAML::Node child_game_object_node = transform_children[i];
		game_object::decode(child_game_object_node, g->transform.id, defered_component_init);
	}

	if (root_decode)
	{
		for (auto &i : *defered_component_init)
		{
			i();
		}
		delete defered_component_init;
		transform_map.clear();
	}
}

void newGameObject(transform2 t)
{
	// new game_object(t);
	int ref = game_object_cache._new();
	game_object_cache.get(ref).transform = t;
	// ref->transform = t;
	t->setGameObject(ref);
}

void rebuildGameObject(componentStorageBase *base, int i)
{
	base->get(i)->transform->gameObject()->components.emplace(base->hash, i);
}

// void registerProto(game_object_proto_ *p)
// {
// 	p->genID();
// 	prototypeRegistry.emplace(pair<int, game_object_proto_ *>(p->id, p));

void gameObjectProtoManager::_new()
{
	shared_ptr<game_object_proto_> p = make_shared<game_object_proto_>();
	p->genID();
	p->name = "prototype_" + to_string(p->id) + ".gop";
	meta[p->id] = p;
	path[p->name] = p->id;
}

game_object_prototype::game_object_prototype() {}
game_object_prototype::game_object_prototype(game_object_proto_ *p)
{
	this->id = p->id;
}

void _child_instatiate(game_object &g, transform2 parent)
{
	// gameLock.lock();
	int ref = game_object_cache._new();
	game_object *ret = &game_object_cache.get(ref);
	ret->destroyed = false;
	// this->transform = new Transform(*g.transform, this);
	// g.transform->getParent()->Adopt(this->transform);

	ret->transform = Transforms._new(); // new Transform(this);
	ret->transform->init(g.transform, ref);
	parent.adopt(ret->transform);
	for (transform2 t : g.transform->getChildren())
	{
		_child_instatiate(*t->gameObject(), ret->transform);
		// new game_object(*t->gameObject(), transform);
	}
	// g.transform.getParent().adopt(this->transform);

	for (auto &i : g.components)
	{
		// game_object::_getComponent(i)->_copy(ret);
		int comp_ref = ComponentRegistry.meta_types.at(i.first)->copy(i.second);
		ComponentRegistry.registry(i.first)->get(comp_ref)->transform = ret->transform;
		ret->components.emplace(i.first, comp_ref);
	}
	for (auto &i : ret->components)
	{
		game_object::_getComponent(i)->init(i.second);
	}
}

game_object *_instantiate(game_object &g)
{
	// game_object *ret = new game_object();
	int ref = game_object_cache._new();
	game_object *ret = &game_object_cache.get(ref);
	ret->destroyed = false;
	// this->transform = new Transform(*g.transform, this);
	// g.transform->getParent()->Adopt(this->transform);

	ret->transform = Transforms._new(); // new Transform(this);
	ret->transform->init(g.transform, ref);
	g.transform.getParent().adopt(ret->transform);
	for (transform2 t : g.transform->getChildren())
	{
		_child_instatiate(*t->gameObject(), ret->transform);
	}
	// g.transform.getParent().adopt(this->transform);

	for (auto &i : g.components)
	{
		int comp_ref = ComponentRegistry.meta_types.at(i.first)->copy(i.second);
		ComponentRegistry.registry(i.first)->get(comp_ref)->transform = ret->transform;
		ret->components.emplace(i.first, comp_ref);
	}
	for (auto &i : ret->components)
	{
		game_object::_getComponent(i)->init(i.second);
	}
	return ret;
}
game_object *instantiate(game_object &g)
{
	game_object *ret = _instantiate(g);
	game_object::startComponents(*ret);
	return ret;
	// for (auto &i : ret->components)
	// {
	// 	i.first->onStart();
	// }
}

game_object *_instantiate()
{
	int ref = game_object_cache._new();
	game_object *ret = &game_object_cache.get(ref);
	ret->destroyed = false;
	// this->transform = new Transform(this);
	ret->transform = Transforms._new();
	ret->transform->init(ref);
	return ret;
	// root->Adopt(this->transform);
}

game_object *_instantiate(game_object_prototype &g)
{
	game_object_proto_ &_g = *game_object_proto_manager.meta.at(g.id);
	int ref = game_object_cache._new();
	game_object *ret = &game_object_cache.get(ref);
	ret->destroyed = false;
	// gameLock.lock();
	ret->transform = Transforms._new(); // new Transform(this);
	ret->transform->init(ref);
	// gameLock.unlock();
	for (auto &i : _g.components)
	{
		// i.first->_copy(ret);
		ret->components.emplace(i.second, ComponentRegistry.meta_types.at(i.second)->copy(i.first));
	}
	for (auto &i : ret->components)
	{
		game_object::_getComponent(i)->transform = ret->transform; // todo // better?
		game_object::_getComponent(i)->init(i.second);
		// toStart.emplace(i.first);
		// i.first->onStart();
	}
	return ret;
}
game_object *instantiate(game_object_prototype &g)
{
	game_object *ret = _instantiate(g);
	for (auto &i : ret->components)
	{
		// i.first->init();
		// toStart.emplace(i.first);
		game_object::_getComponent(i)->onStart();
	}
	return ret;
}

game_object *instantiate()
{
	return _instantiate();
}

game_object *transform2::gameObject()
{
	return &game_object_cache.get(Transforms.meta[id].gameObject);
}

void transform2::setGameObject(int g)
{
	Transforms.meta[id].gameObject = g;
}

void game_object::_destroy()
{

	for (auto i : components)
	{
		game_object::_getComponent(i)->onDestroy();
		game_object::_getComponent(i)->deinit(i.second);
	}
	for (auto &c : components)
	{
		ComponentRegistry.registry(c.first)->erase(c.second);
	}
	while (transform->getChildren().size() > 0)
	{
		transform->getChildren().front()->gameObject()->_destroy();
	}
	game_object_cache._delete(Transforms.meta[transform].gameObject);
	transform->_destroy();
}

game_object_proto_ *game_object_prototype::meta() const
{
	return game_object_proto_manager.meta.at(id).get();
}

void renderEdit(const char *name, game_object_prototype &g)
{
	// if (g.id < 1) // uninitialized
	// 	ImGui::InputText(name, "", 1, ImGuiInputTextFlags_ReadOnly);
	// else
	// 	ImGui::InputText(name, (char *)g.meta()->name.c_str(), g.meta()->name.size() + 1, ImGuiInputTextFlags_ReadOnly);
	AssetRenderEdit(name, &g);
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("GAME_OBJECT_PROTO_TYPE"))
		{
			IM_ASSERT(payload->DataSize == sizeof(int));
			int payload_n = *(const int *)payload->Data;
			g.id = payload_n;
		}
		ImGui::EndDragDropTarget();
	}
}

// template<>6
YAML::Node YAML::convert<game_object_proto_>::encode(const game_object_proto_ &rhs)
{
	Node node;
	YAML_ENCODE_ASSET();
	// components
	YAML::Node components_node;
	for (auto &i : rhs.components)
	{
		YAML::Node component_node;
		component_node["id"] = i.second;
		YAML::Node component_val_node;
		i.first->ser_edit(ser_mode::write_mode, component_val_node);
		component_node["value"] = component_val_node;
		components_node.push_back(component_node);
	}
	YAML::Node out;
	out["components"] = components_node;
	ofstream(rhs.name) << out;
	// node["components"] = components_node;
	return node;
}

bool YAML::convert<game_object_proto_>::decode(const Node &node, game_object_proto_ &rhs)
{
	YAML_DECODE_ASSET();
	YAML::Node in = YAML::LoadFile(rhs.name);
	YAML::Node components_node = in["components"];
	for (int i = 0; i < components_node.size(); i++)
	{
		YAML::Node component_node = components_node[i];
		size_t hash = component_node["id"].as<size_t>();
		YAML::Node component_val_node = component_node["value"];

		if (hash == typeid(placeholder).hash_code() && ComponentRegistry.registry(component_val_node["data"]["id"].as<size_t>()) != 0)
		{
			hash = component_val_node["data"]["id"].as<size_t>();
			component_val_node = component_val_node["data"]["value"];
		} // if registry does not contain component placeholder is referencing then placeholder is de-serialized

		if (ComponentRegistry.registry(hash))
		{
			component *c = ComponentRegistry.registry(hash)->_decode(component_val_node);
			rhs.components.emplace(c, hash);
		}
		else
		{
			placeholder *p = new placeholder();
			p->data = component_node;

			rhs.components.emplace(p, typeid(placeholder).hash_code());
		}
	}
	return true;
}
