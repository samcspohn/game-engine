#include "component.h"
void component::onStart() {}
void component::onDestroy() {}
void component::deinit() {}
void component::init() {}
bool component::_registerEngineComponent() { return false; };
// void component::onCollision(game_object *go, glm::vec3 point,  glm::vec3 normal){};
// void component::_update(int index, unsigned int _start, unsigned int _end){};
void component::update(){};
void component::lateUpdate(){};
int component::getThreadID()
{
	return tbb::this_task_arena::current_thread_index();
}
size_t component::getHash()
{
	return typeid(*this).hash_code();
}
