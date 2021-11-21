#include "components/Component.h"

vector<int> numbers = {0};
void component::onStart() {}
void component::onDestroy() {}
void component::deinit(int id) {}
void component::init(int id) {}
bool component::_registerEngineComponent() { return false; };
void component::onCollision(collision& c){};
void component::update(){};
void component::lateUpdate(){};
int component::getThreadID()
{
	// return 0;
	return tbb::this_task_arena::current_thread_index();
}
size_t component::getHash()
{
	return typeid(*this).hash_code();
}
Registry ComponentRegistry;

void destroyAllComponents(){
	// ComponentRegistry.components.clear();
	// while (ComponentRegistry.components.size() > 0){
	// 	delete ComponentRegistry.components.begin()->second;
	// 	ComponentRegistry.components.erase(ComponentRegistry.components.begin());
	// }
}
