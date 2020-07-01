#include "Transform.h"
using namespace std;

mutex gameLock;
glm::vec3 vec3forward(0, 0, 1);
glm::vec3 vec3up(0, 1, 0);
glm::vec3 vec3right(1, 0, 0);
glm::vec3 mainCamPos;
glm::vec3 mainCamUp;
glm::vec3 MainCamForward;

void _transform::translate(glm::vec3 translation) {
	position += rotation * translation;
}
void _transform::rotate(glm::vec3 axis, float radians) {
	rotation = glm::rotate(rotation, radians, axis);
}

atomic<int> GPU_TRANSFORMS_UPDATES_itr;
deque_heap<_transform> TRANSFORMS;
deque_heap<_transform> STATIC_TRANSFORMS;
gpu_vector_proxy<_transform>* GPU_TRANSFORMS;
gpu_vector_proxy<GLuint>* transformIds;
gpu_vector_proxy<_transform>* GPU_TRANSFORMS_UPDATES;

void initTransform(){
	GPU_TRANSFORMS = new gpu_vector_proxy<_transform>();
	GPU_TRANSFORMS_UPDATES = new gpu_vector_proxy<_transform>();
	transformIds = new gpu_vector_proxy<GLuint>();
}

int switchAH(int index) {
	return ~index - 1;
}
unsigned int transforms_enabled = 0;
_transform& get_T(int index) {
	return (index >= 0 ? TRANSFORMS[index] : STATIC_TRANSFORMS[switchAH(index)]);
}

Transform* root;

void Transform::init() {
	// gameLock.lock();
	_T = TRANSFORMS._new();
	// gameLock.unlock();
	parent = 0;
	root->Adopt(this);
}
Transform::Transform(game_object* g) {
	this->gameObject = g;
	init();
}

Transform::Transform(Transform& other, game_object* go) {
	this->gameObject = go;
	init();
	_T->position = other.getPosition();
	_T->rotation = other.getRotation();
	_T->scale = other.getScale();
}

Transform Transform::operator=(const Transform& t) {
	this->_T = t._T;
	this->gameObject = t.gameObject;
	this->children = t.children;
	this->enabled = t.enabled;
	this->parent = t.parent;
}

void Transform::lookat(glm::vec3 lookatPoint, glm::vec3 up) {
	glm::quat r = glm::toQuat(glm::lookAt(_T->position, lookatPoint, up));
	_T->rotation = r;
}
glm::vec3 Transform::forward() {
	return glm::normalize(_T->rotation * glm::vec3(0.0f, 0.0f, 1.0f));
}
glm::vec3 Transform::right() {
	return glm::normalize(_T->rotation * glm::vec3(1.0f, 0.0f, 0.0f));
}
glm::vec3 Transform::up() {
	return glm::normalize(_T->rotation * glm::vec3(0.0f, 1.0f, 0.0f));
}
glm::mat4 Transform::getModel() {
	return (glm::translate(_T->position) * glm::scale(_T->scale))* glm::toMat4(_T->rotation);
}
glm::vec3 Transform::getScale() {
	return _T->scale;
}
void Transform::setScale(glm::vec3 scale) {
	this->scale(scale / _T->scale);
}
glm::vec3 Transform::getPosition() {
	return _T->position;
}
void Transform::setPosition(glm::vec3 pos) {
	// m.lock();
	for (Transform* c : children)
		c->translate(pos - _T->position, glm::quat());
	_T->position = pos;
	// m.unlock();
}
glm::quat Transform::getRotation() {
	return _T->rotation;
}
void Transform::setRotation(glm::quat r) {
	_T->rotation = r;
}
list<Transform*>& Transform::getChildren() {
	return children;
}
Transform* Transform::getParent() {
	return parent;
}
void Transform::Adopt(Transform * transform) {
	if(transform->parent == this)
		return;
	transform->orphan();
	transform->parent = this;
	this->m.lock();
	this->children.push_back(transform);
	transform->childId = (--this->children.end());
	this->m.unlock();
}

void Transform::_destroy() {
	orphan();
	if (enabled) {
		TRANSFORMS._delete(_T);
	}
	else
	{
		STATIC_TRANSFORMS._delete(_T);
	}
	if (enabled)
		--transforms_enabled;
	delete this;
}

void Transform::move(glm::vec3 movement, bool hasChildren = false) {
	_T->position += movement;
	if(hasChildren){
		for (auto a : children)
		a->translate(movement, glm::quat(1,0,0,0));
	}
}
void Transform::translate(glm::vec3 translation) {
	// m.lock();
	_T->position += _T->rotation * translation;
	for (auto a : children)
		a->translate(translation, _T->rotation);
	// m.unlock();
}
void Transform::translate(glm::vec3 translation, glm::quat r) {
	// m.lock();
	_T->position += r * translation;
	for (Transform* a : children)
		a->translate(translation, r);
	// m.unlock();
}
void Transform::scale(glm::vec3 scale) {
	// m.lock();
	_T->scale *= scale;
	for (Transform* a : children)
		a->scaleChild(_T->position, scale);
	// m.unlock();
}
void Transform::scaleChild(glm::vec3 pos, glm::vec3 scale) {
	// m.lock();
	_T->position = (_T->position - pos) * scale + pos;
	_T->scale *= scale;
	for (Transform* a : children)
		a->scaleChild(pos, scale);
	// m.unlock();
}
void Transform::rotate(glm::vec3 axis, float radians) {
	// m.lock();
	_T->rotation = glm::rotate(_T->rotation, radians, axis);
	for (Transform* a : children)
		a->rotateChild(axis, _T->position, _T->rotation, radians);
	_T->rotation = normalize(_T->rotation);
	// m.unlock();
}
void Transform::rotateChild(glm::vec3 axis, glm::vec3 pos, glm::quat r, float angle) {
	// m.lock();
	glm::vec3 ax = r * axis;
	_T->position = pos + glm::rotate(_T->position - pos, angle, ax);
	_T->rotation = glm::rotate(_T->rotation, angle, glm::inverse(_T->rotation) * ax);// glm::rotate(rotation, angle, axis);
	for (Transform* a : children)
		a->rotateChild(axis, pos, r, angle);
	_T->rotation = normalize(_T->rotation);
	// m.unlock();
}


Transform::~Transform() {	}
Transform::Transform(Transform & other) {
}

void Transform::orphan() {
	if (this->parent == 0 )
		return;
	this->parent->m.lock();
	this->parent->children.erase(childId);
	this->parent->m.unlock();
	this->parent = 0;
}


bool compareTransform(Transform* t1, Transform* t2){
	return t1->_T < t2->_T;
}
