#include "game_object.h"

tbb::concurrent_unordered_set<game_object*> toDestroy;
std::unordered_map<int, game_object_proto_*> prototypeRegistry;

void newGameObject(transform2 t){
	new game_object(t);
}
REGISTER_ASSET(game_object_proto_);

void rebuildGameObject(componentStorageBase* base, int i){
    base->get(i)->transform->gameObject()->components.insert(std::make_pair(base->get(i), base->getInfo(i).CompItr));
}

void registerProto(game_object_proto_* p){
	p->genID();
	prototypeRegistry.emplace(pair<int,game_object_proto_*>(p->id,p));
	// p->ref = prototypeRegistry.end();
	assets::registerAsset(p);
    // --p->ref;
}
void deleteProtoRef(int id){
	prototypeRegistry.erase(id);
}


void saveProto(OARCHIVE& oa){
	oa << prototypeRegistry;
}
void loadProto(IARCHIVE& ia){
	ia >> prototypeRegistry;
}


void componentMetaBase::addComponent(game_object* g){}
void componentMetaBase::addComponentProto(game_object_proto_* g){}


game_object_prototype::game_object_prototype(){}
game_object_prototype::game_object_prototype(game_object_proto_* p){
	this->id = p->id;
}
