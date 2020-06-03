#pragma once

#include "array_heap.h"
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <mutex>
#include "Component.h"
#include "game_object.h"
#include "gpu_vector.h"
// ~-1 = 0

using namespace std;

mutex gameLock;
glm::vec3 vec3forward(0, 0, 1);
glm::vec3 vec3up(0, 1, 0);
glm::vec3 vec3right(1, 0, 0);
glm::vec3 mainCamPos;
glm::vec3 mainCamUp;
glm::vec3 MainCamForward;

struct _transform {
	glm::vec3 position; GLint id = -1;
	glm::vec3 scale = glm::vec3(1); GLint parent = 0;
	glm::quat rotation = glm::quat(1,0,0,0);
	void translate(glm::vec3 translation) {
		position += rotation * translation;
	}
	void rotate(glm::vec3 axis, float radians) {
		rotation = glm::rotate(rotation, radians, axis);
//		for (Transform* a : children)
//			a->rotateChild(axis, position, rotation, radians);
//		rotation = normalize(rotation);
	}
};
deque_heap<_transform> TRANSFORMS;// = array_heap<_transform>();
deque_heap<_transform> STATIC_TRANSFORMS;// = array_heap<_transform>();
gpu_vector<_transform>* GPU_TRANSFORMS = nullptr;

class game_object;
class {
	mutex m;
public:
	vector<game_object*> data = vector<game_object*>();
	game_object*& operator[](int index) {
		if (index > TRANSFORMS.size())
			throw;
		if (index >= data.size()) {
			m.lock();
			if (index >= data.size()) {
				data.resize(index + 1);
			}
			m.unlock();
		}
		return data[index];
	}
}GO_T_refs;

int switchAH(int index) {
	return ~index - 1;
}
unsigned int transforms_enabled = 0;
_transform& get_T(int index) {
	return (index >= 0 ? TRANSFORMS[index] : STATIC_TRANSFORMS[switchAH(index)]);
}

Transform* root;
class Transform {
	mutex m;
public:
	deque_heap<_transform>::ref _T;
	game_object* gameObject;

	void init() {
		// gameLock.lock();
		_T = TRANSFORMS._new();
		GO_T_refs[_T] = gameObject;
		// gameLock.unlock();
		parent = 0;
		root->Adopt(this);
	}
	Transform(game_object* g) {
		this->gameObject = g;
		init();
	}

	Transform(Transform& other, game_object* go) {
		this->gameObject = go;
		init();
		_T->position = other.getPosition();
		_T->rotation = other.getRotation();
		_T->scale = other.getScale();
	}

	Transform operator=(const Transform& t) {
		this->_T = t._T;
		this->gameObject = t.gameObject;
		this->children = t.children;
		this->enabled = t.enabled;
		this->parent = t.parent;
	}

	void lookat(glm::vec3 lookatPoint, glm::vec3 up) {
		glm::quat r = glm::toQuat(glm::lookAt(_T->position, lookatPoint, up));
		_T->rotation = r;
	}
	glm::vec3 forward() {
		return glm::normalize(_T->rotation * glm::vec3(0.0f, 0.0f, 1.0f));
	}
	glm::vec3 right() {
		return glm::normalize(_T->rotation * glm::vec3(1.0f, 0.0f, 0.0f));
	}
	glm::vec3 up() {
		return glm::normalize(_T->rotation * glm::vec3(0.0f, 1.0f, 0.0f));
	}
	glm::mat4 getModel() {
		return (glm::translate(_T->position) * glm::scale(_T->scale))* glm::toMat4(_T->rotation);
	}
	glm::vec3 getScale() {
		return _T->scale;
	}
	void setScale(glm::vec3 scale) {
		this->scale(scale / _T->scale);
	}
	glm::vec3 getPosition() {
		return _T->position;
	}
	void setPosition(glm::vec3 pos) {
		m.lock();
		for (Transform* c : children)
			c->translate(pos - _T->position, glm::quat());
		_T->position = pos;
		m.unlock();
	}
	glm::quat getRotation() {
		return _T->rotation;
	}
	void setRotation(glm::quat r) {
		_T->rotation = r;
	}
	list<Transform*>& getChildren() {
		return children;
	}
	Transform* getParent() {
		return parent;
	}
	void Adopt(Transform * transform) {
	    if(transform->parent == this)
            return;
		transform->orphan();
		transform->parent = this;
		this->m.lock();
		this->children.push_back(transform);
		transform->childId = (--this->children.end());
		this->m.unlock();
	}

	void _destroy() {
		orphan();
		if (enabled) {
			GO_T_refs[_T] = 0;
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
	
	void move(glm::vec3 lhs) {
		setPosition(_T->position + lhs);
	}
	void translate(glm::vec3 translation) {
		m.lock();
		_T->position += _T->rotation * translation;
		for (auto a : children)
			a->translate(translation, _T->rotation);
		m.unlock();
	}
	void translate(glm::vec3 translation, glm::quat r) {
		m.lock();
		_T->position += r * translation;
		for (Transform* a : children)
			a->translate(translation, r);
		m.unlock();
	}
	void scale(glm::vec3 scale) {
		m.lock();
		_T->scale *= scale;
		for (Transform* a : children)
			a->scaleChild(_T->position, scale);
		m.unlock();
	}
	void scaleChild(glm::vec3 pos, glm::vec3 scale) {
		m.lock();
		_T->position = (_T->position - pos) * scale + pos;
		_T->scale *= scale;
		for (Transform* a : children)
			a->scaleChild(pos, scale);
		m.unlock();
	}
    void rotate(glm::vec3 axis, float radians) {
		m.lock();
		_T->rotation = glm::rotate(_T->rotation, radians, axis);
		for (Transform* a : children)
			a->rotateChild(axis, _T->position, _T->rotation, radians);
		_T->rotation = normalize(_T->rotation);
		m.unlock();
	}
	void rotateChild(glm::vec3 axis, glm::vec3 pos, glm::quat r, float angle) {
		m.lock();
		glm::vec3 ax = r * axis;
		_T->position = pos + glm::rotate(_T->position - pos, angle, ax);
		_T->rotation = glm::rotate(_T->rotation, angle, glm::inverse(_T->rotation) * ax);// glm::rotate(rotation, angle, axis);
		for (Transform* a : children)
			a->rotateChild(axis, pos, r, angle);
		_T->rotation = normalize(_T->rotation);
		m.unlock();
	}

private:
	bool enabled = true;
	Transform * parent;
	list<Transform*> children;
	list<Transform*>::iterator childId;

	friend void destroyRoot(Transform * t);
	~Transform() {	}
	Transform(Transform & other) {
	}

	void orphan() {
		if (this->parent == 0 )
			return;
		this->parent->m.lock();
		this->parent->children.erase(childId);
		this->parent->m.unlock();
		this->parent = 0;
	}


};

bool compareTransform(Transform* t1, Transform* t2){
    return t1->_T < t2->_T;
}
