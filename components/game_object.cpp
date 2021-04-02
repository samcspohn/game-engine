#include "game_object.h"

std::mutex toDestroym;
// std::deque<game_object *> toDestroyGameObjects;
std::vector<game_object *> toDestroyGameObjects;
// tbb::concurrent_vector<game_object *> toDestroyGameObjects;
std::unordered_map<int, game_object_proto_ *> prototypeRegistry;
STORAGE<game_object> game_object_cache;

void game_object::serialize(OARCHIVE &ar, game_object *g)
{
	ar << g->transform.id;
	ar << g->transform->getPosition();
	ar << g->transform->getRotation();
	ar << g->transform->getScale();
	ar << g->transform->getParent().id;

	ar << g->components.size();
	for (auto &i : g->components)
	{
		ar << i.first;
		ComponentRegistry.registry(i.first)->serialize(ar, i.second);
	}
	ar << g->transform->getChildren().size();
	for (auto &i : g->transform->getChildren())
	{
		serialize(ar, i->gameObject());
	}
}
void game_object::deserialize(IARCHIVE &ar, map<int, int> &transform_map)
{

	int ref = game_object_cache._new();
	game_object *g = &game_object_cache.get(ref);
	int transformID;
	glm::vec3 p;
	glm::quat r;
	int parentID;
	int size;

	ar >> transformID;
	g->transform = Transforms._new();
	transform_map[transformID] = g->transform.id;
	ar >> p;
	g->transform->setPosition(p);
	ar >> r;
	g->transform->setRotation(r);
	ar >> p;
	g->transform->setScale(p);
	ar >> parentID;

	Transforms.meta[g->transform.id].gameObject = ref;
	if(parentID != -1)
		transform2(transform_map[parentID]).adopt(g->transform);

	ar >> size;
	for (int i = 0; i < size; i++)
	{
		size_t hash;
		ar >> hash;
		int id = ComponentRegistry.registry(hash)->deserialize(ar);
		g->components.emplace(hash, id);
		_getComponent(pair<size_t, int>(hash, id))->transform = g->transform;
	}
	ar >> size;
	for (int i = 0; i < size; i++)
	{
		deserialize(ar, transform_map);
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
	assets::registerAsset(p);
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
		game_object::_getComponent(i)->_copy(ret);
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
		game_object::_getComponent(i)->_copy(ret);
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
		i.first->_copy(ret);
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