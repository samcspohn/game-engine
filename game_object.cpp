#include "game_object.h"

tbb::concurrent_unordered_set<game_object*> toDestroy;
std::list<game_object_proto*> prototypeRegistry;

void newGameObject(transform2 t){
	new game_object(t);
}
REGISTER_ASSET(game_object_proto);

void rebuildGameObject(componentStorageBase* base, int i){
    base->get(i)->transform->gameObject()->components.insert(std::make_pair(base->get(i), base->getInfo(i).CompItr));
}

void registerProto(game_object_proto* p){
	prototypeRegistry.push_back(p);
	p->ref = prototypeRegistry.end();
	p->genID();
	assets::registerAsset(p);
    --p->ref;
}
void deleteProtoRef(protoListRef r){
	prototypeRegistry.erase(r);
}


void saveProto(OARCHIVE& oa){
	oa << prototypeRegistry;
}
void loadProto(IARCHIVE& ia){
	ia >> prototypeRegistry;
}


void componentMetaBase::addComponent(game_object* g){}

