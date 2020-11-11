#include "game_object.h"

tbb::concurrent_unordered_set<game_object*> toDestroy;

void newGameObject(transform2 t){
	new game_object(t);
}

void rebuildGameObject(componentStorageBase* base, int i){
    base->get(i)->transform->gameObject()->components.insert(std::make_pair(base->get(i), base->getInfo(i).CompItr));
}