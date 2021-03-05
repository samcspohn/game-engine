#include "game_object.h"

tbb::concurrent_unordered_set<game_object *> toDestroyGameObjects;
std::unordered_map<int, game_object_proto_ *> prototypeRegistry;

void newGameObject(transform2 t)
{
	new game_object(t);
}
REGISTER_ASSET(game_object_proto_);

void rebuildGameObject(componentStorageBase *base, int i)
{
	base->get(i)->transform->gameObject()->components.insert(std::make_pair(base->get(i), base->getInfo(i).CompItr));
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

game_object *_instantiate(game_object &g)
{
	game_object *ret = new game_object();
	ret->destroyed = false;
	// this->transform = new Transform(*g.transform, this);
	// g.transform->getParent()->Adopt(this->transform);

	ret->transform = Transforms._new(); // new Transform(this);
	ret->transform->init(g.transform, ret);
	g.transform.getParent().adopt(ret->transform);
	for (transform2 t : g.transform->getChildren())
	{
		new game_object(*t->gameObject(), ret->transform);
	}
	// g.transform.getParent().adopt(this->transform);

	for (auto &i : g.components)
	{
		i.first->_copy(ret);
	}
	for (auto &i : ret->components)
	{
		i.first->init();
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
	game_object *ret = new game_object();
	ret->destroyed = false;
	// this->transform = new Transform(this);
	ret->transform = Transforms._new();
	ret->transform->init(ret);
	return ret;
	// root->Adopt(this->transform);
}

game_object *_instantiate(game_object_prototype &g)
{
	game_object_proto_ &_g = *prototypeRegistry.at(g.id);
	game_object *ret = new game_object();
	ret->destroyed = false;
	// gameLock.lock();
	ret->transform = Transforms._new(); // new Transform(this);
	ret->transform->init(ret);
	// gameLock.unlock();
	for (auto &i : _g.components)
	{
		i.first->_copy(ret);
	}
	for (auto &i : ret->components)
	{
		i.first->init();
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
		i.first->onStart();
	}
	return ret;
}

game_object *instantiate()
{
	return _instantiate();
}