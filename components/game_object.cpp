#include "game_object.h"

std::mutex toDestroym;
// std::deque<game_object *> toDestroyGameObjects;
std::vector<game_object *> toDestroyGameObjects;
// tbb::concurrent_vector<game_object *> toDestroyGameObjects;
std::unordered_map<int, shared_ptr<game_object_proto_>> prototypeRegistry;
STORAGE<game_object> game_object_cache;


void encodePrototypes(YAML::Node& node){
	YAML::Node prototypeRegistry_node;
	for(auto& i : prototypeRegistry){
		prototypeRegistry_node[i.first] = *i.second;
	}
	node["prototype_registry"] = prototypeRegistry_node;
}


void decodePrototypes(YAML::Node& node){
	YAML::Node prototypeRegistry_node = node["prototype_registry"];
	for(YAML::const_iterator i = prototypeRegistry_node.begin(); i != prototypeRegistry_node.end(); ++i){
		prototypeRegistry[i->first.as<int>()] = make_shared<game_object_proto_>(i->second.as<game_object_proto_>());
	}
}

void game_object::encode(YAML::Node &game_object_node, game_object *g)
{
	// transform
	YAML::Node transform_node;
	transform_node["position"] = g->transform->getPosition();
	transform_node["rotation"] = g->transform->getRotation();
	transform_node["scale"] = g->transform->getScale();
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
void game_object::decode(YAML::Node &game_object_node, int parent_id)
{
	int ref = game_object_cache._new();
	game_object *g = &game_object_cache.get(ref);

	g->transform = Transforms._new();
	g->transform->setPosition(game_object_node["transform"]["position"].as<glm::vec3>());
	g->transform->setRotation(game_object_node["transform"]["rotation"].as<glm::quat>());
	g->transform->setScale(game_object_node["transform"]["scale"].as<glm::vec3>());

	Transforms.meta[g->transform.id].gameObject = ref;
	if (parent_id != -1)
		transform2(parent_id).adopt(g->transform);

	YAML::Node components_node = game_object_node["components"];
	for (int i = 0; i < components_node.size(); i++)
	{
		YAML::Node component_node = components_node[i];
		size_t hash = component_node["id"].as<size_t>();
		YAML::Node component_val_node = component_node["value"];
		int id = ComponentRegistry.registry(hash)->decode(component_val_node);
		g->components.emplace(hash, id);
		_getComponent(pair<size_t, int>(hash, id))->transform = g->transform;
	}

	YAML::Node transform_children = game_object_node["transform"]["children"];
	for (int i = 0; i < transform_children.size(); i++)
	{
		YAML::Node child_game_object_node = transform_children[i];
		game_object::decode(child_game_object_node, ref);
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
REGISTER_ASSET(game_object_proto_);

void rebuildGameObject(componentStorageBase *base, int i)
{
	base->get(i)->transform->gameObject()->components.emplace(base->hash, i);
}

void registerProto(game_object_proto_ *p)
{
	p->genID();
	prototypeRegistry.emplace(pair<int, game_object_proto_ *>(p->id, p));
	// p->ref = prototypeRegistry.end();
	// assets::registerAsset(p);
	// --p->ref;
}
void deleteProtoRef(int id)
{
	prototypeRegistry.erase(id);
}

void saveProto(OARCHIVE &oa)
{
	oa << prototypeRegistry;
}
void loadProto(IARCHIVE &ia)
{
	ia >> prototypeRegistry;
}

void componentMetaBase::addComponent(game_object *g) {}
void componentMetaBase::addComponentProto(game_object_proto_ *g) {}

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
		ret->components.emplace(i.first,ComponentRegistry.getByType(i.first)->copy(i.second));
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
		ret->components.emplace(i.first,ComponentRegistry.getByType(i.first)->copy(i.second));
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
	game_object_proto_ &_g = *prototypeRegistry.at(g.id);
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
		ret->components.emplace(i.second,ComponentRegistry.getByType(i.second)->copy(i.first));
	}
	for (auto &i : ret->components)
	{
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