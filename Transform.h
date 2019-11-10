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

glm::vec3 vec3forward(0, 0, 1);
glm::vec3 vec3up(0, 1, 0);
glm::vec3 vec3right(1, 0, 0);
glm::vec3 mainCamPos;
glm::vec3 MainCamForward;

struct _transform {
	glm::vec3 position; GLint id = -1;
	glm::vec3 scale = glm::vec3(1); GLint parent = 0;
	glm::quat rotation = glm::quat(1,0,0,0);
	GLint childrenBegin = -1;
	GLint childrenEnd = -1;
	GLint prevSibling = -1;
	GLint nextSibling = -1;
};
array_heap<_transform> TRANSFORMS = array_heap<_transform>();
array_heap<_transform> STATIC_TRANSFORMS = array_heap<_transform>();
gpu_vector<_transform>* GPU_TRANSFORMS = nullptr;

class game_object;
class {
public:
	vector<game_object*> data = vector<game_object*>();
	game_object*& operator[](int index) {
		if (index >= data.size()) {
			if (index > TRANSFORMS.size())
				throw;
			data.resize(index + 1);
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

mutex transformArray;
mutex transformMutex;

Transform* root;
class Transform {
public:
	array_heap<_transform>::ref _T;
	game_object* gameObject;

	void init() {
		transformMutex.lock();
		_T = TRANSFORMS._new();
		GO_T_refs[_T] = gameObject;
		transformMutex.unlock();
		parent = 0;
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

	void translate(glm::vec3 translation) {
		translation *= _T->scale;
		_T->position += _T->rotation * translation;
		for (auto a : children)
			a->translate(translation, _T->rotation);
	}
	void translate(glm::vec3 translation, glm::quat r) {
		_T->position += r * translation;
		for (Transform* a : children)
			a->translate(translation, r);
	}
	void setPosition(glm::vec3 pos) {
		for (Transform* c : children)
			c->translate(pos - _T->position, glm::quat());
		_T->position = pos;
	}
	glm::vec3 getPosition() {
		return _T->position;
	}
	void move(glm::vec3 lhs) {
		setPosition(_T->position + lhs);
	}

	void scale(glm::vec3 scale) {
		_T->scale *= scale;
		for (Transform* a : children)
			a->scaleChild(_T->position, scale);
	}
	glm::vec3 getScale() {
		return _T->scale;
	}
	void setScale(glm::vec3 scale) {
		this->scale(scale / _T->scale);
	}

	void rotate(glm::vec3 axis, float degress) {
		//axis = glm::normalize(rotation * axis);
		degress = glm::radians(degress);
		_T->rotation = glm::rotate(_T->rotation, degress, axis);
		for (Transform* a : children)
			a->rotateChild(axis, _T->position, _T->rotation, degress);
		_T->rotation = normalize(_T->rotation);
	}
	glm::quat getRotation() {
		return _T->rotation;
	}
	void setRotation(glm::quat r) {
		_T->rotation = r;
	}
	plf::list<Transform*>& getChildren() {
		return children;
	}
	Transform* getParent() {
		return parent;
	}
	void Adopt(Transform * transform) {
		// if(children.find(transform) != children.end())
		// 	return;
		transform->orphan();
		transform->parent = this;
//		transform->_T->parent = _T;

//		int _Tval = (transform->enabled ? transform->_T : switchAH(_T));
//		if (children.size() == 0) {
//			_T->childrenBegin = _Tval;
//		}
//		else {
//			transform->_T->prevSibling = _T->childrenEnd;
//			get_T(_T->childrenEnd).nextSibling = _Tval;
//		}
//		_T->childrenEnd = _Tval;
		this->children.push_back(transform);
		transform->childId = (--this->children.end());
	}
	void rotateChild(glm::vec3 axis, glm::vec3 pos, glm::quat r, float angle) {
		glm::vec3 ax = r * axis;
		_T->position = pos + glm::rotate(_T->position - pos, angle, ax);
		_T->rotation = glm::rotate(_T->rotation, angle, glm::inverse(_T->rotation) * ax);// glm::rotate(rotation, angle, axis);
		for (Transform* a : children)
			a->rotateChild(axis, pos, r, angle);
		_T->rotation = normalize(_T->rotation);
	}

	void _destroy() {
//		transformMutex.lock();
		orphan();
		if (enabled) {
			GO_T_refs[_T] = 0;
			TRANSFORMS._delete(_T);
		}
		else
		{
			STATIC_TRANSFORMS._delete(_T);
		}

//		transformMutex.unlock();
		if (enabled)
			--transforms_enabled;
		delete this;
	}

	//void enable() {
	//	if (!enabled) {
	//		transformMutex.lock();

	//		auto a = TRANSFORMS._new();
	//		GO_T_refs[a] = gameObject;
	//		a.data() = _T.data();
	//		int _Tval = switchAH(_T);

	//		//_T is in STATIC_TRANSFORMS

	//		//parent
	//		//parent children begin
	//		//if (_T->parent != -1 && get_T(_T->parent).childrenBegin == _Tval)
	//		//	get_T(_T->parent).childrenBegin = a;

	//		////parent children end
	//		//if (_T->parent != -1 && get_T(_T->parent).childrenEnd == _Tval)
	//		//	get_T(_T->parent).childrenEnd = a;

	//		//// siblings
	//		////prevsibling
	//		//if (_T->prevSibling != -1)
	//		//	get_T(_T->prevSibling).nextSibling = a;

	//		////nextsibling
	//		//if (_T->nextSibling != -1)
	//		//	get_T(_T->nextSibling).prevSibling = a;

	//		////children
	//		//for (int i = _T->childrenBegin; i != -1; i = get_T(i).nextSibling)
	//		//	get_T(i).parent = a;

	//		STATIC_TRANSFORMS._delete(_T);
	//		_T = a;

	//		++transforms_enabled;
	//		enabled = true;
	//		transformMutex.unlock();
	//	}
	//}
	//void disable() {
	//	if (enabled) {
	//		transformMutex.lock();
	//		--transforms_enabled;
	//		auto a = STATIC_TRANSFORMS._new();
	//		a.data() = _T.data();

	//		////parent
	//		////parent children begin
	//		//if (_T->parent != -1 && get_T(_T->parent).childrenBegin == _T)
	//		//	get_T(_T->parent).childrenBegin = switchAH(a);
	//		//
	//		////parent children end
	//		//if (_T->parent != -1 && get_T(_T->parent).childrenEnd == _T)
	//		//	get_T(_T->parent).childrenEnd = switchAH(a);

	//		//// siblings
	//		////prevsibling
	//		//if (_T->prevSibling != -1)
	//		//	get_T(_T->prevSibling).nextSibling = switchAH(a);
	//		//
	//		////nextsibling
	//		//if (_T->nextSibling != -1)
	//		//	get_T(_T->nextSibling).prevSibling = switchAH(a);
	//		//
	//		////children
	//		//for (int i = _T->childrenBegin; i != -1; i = get_T(i).nextSibling)
	//		//	get_T(i).parent = switchAH(a);


	//		GO_T_refs[_T] = 0;
	//		TRANSFORMS._delete(_T);
	//		_T = a;
	//		enabled = false;
	//		transformMutex.unlock();
	//	}
	//}

	int shift() {
		//transformMutex.lock();

		_transform t = _T.data();

		auto a = TRANSFORMS._new();
		GO_T_refs[a] = gameObject;
		a.data() = t;
		//parent
		//parent children begin
//		if (_T->parent >= 0 && TRANSFORMS[_T->parent].childrenBegin == _T)
//			TRANSFORMS[_T->parent].childrenBegin = a;
//		else if (_T->parent < -1 && STATIC_TRANSFORMS[-(_T->parent - 2)].childrenBegin == _T)
//			STATIC_TRANSFORMS[-(_T->parent) - 2].childrenBegin = a;
//		//parent children end
//		if (_T->parent >= 0 && TRANSFORMS[_T->parent].childrenEnd == _T)
//			TRANSFORMS[_T->parent].childrenEnd = a;
//		else if (_T->parent < -1 && STATIC_TRANSFORMS[-(_T->parent - 2)].childrenEnd == _T)
//			STATIC_TRANSFORMS[-(_T->parent) - 2].childrenEnd = a;
//
//
//		// siblings
//		//prevsibling
//		if (_T->prevSibling >= 0)
//			TRANSFORMS[_T->prevSibling].nextSibling = a;
//		else if (_T->prevSibling < -1)
//			STATIC_TRANSFORMS[-(_T->prevSibling) - 2].nextSibling = a;
//		//nextsibling
//		if (_T->nextSibling >= 0)
//			TRANSFORMS[_T->nextSibling].prevSibling = a;
//		else if (_T->nextSibling < -1)
//			STATIC_TRANSFORMS[-(_T->nextSibling) - 2].prevSibling = a;
//
//		//children
//		for (int i = _T->childrenBegin; i != -1; i = get_T(i).nextSibling)
//			(i >= 0 ? TRANSFORMS[i] : STATIC_TRANSFORMS[-i - 2]).parent = a;

		GO_T_refs[_T] = 0;
		TRANSFORMS._delete(_T);

		_T = a;
		//transformMutex.unlock();
		return _T;
	}

private:
	bool enabled = true;
	Transform * parent;
	plf::list<Transform*> children;
	plf::list<Transform*>::iterator childId;

	friend void destroyRoot(Transform * t);
	~Transform() {	}
	Transform(Transform & other) {
	}

	void orphan() {
		// || this->parent->children.find(this) == this->parent->children.end()
		if (this->parent == 0 )
			return;
//		//todo add support for static parents
//		_transform * parentT = &(TRANSFORMS[_T->parent]);
//		if (parent->children.size() > 1) {
//			if (parentT->childrenBegin == (enabled ? _T : switchAH(_T))) {//front of children
//				get_T(_T->nextSibling).prevSibling = -1;
//				parentT->childrenBegin = _T->nextSibling;
//			}
//			else if (parentT->childrenEnd == (enabled ? _T : switchAH(_T))) {
//				get_T(_T->prevSibling).nextSibling = -1;
//
//				parentT->childrenEnd = _T->prevSibling;
//			}
//			else if (parent->children.size() > 2){
//				get_T(_T->prevSibling).nextSibling = _T->nextSibling;
//				get_T(_T->nextSibling).prevSibling = _T->prevSibling;
//			}
//		}
//		else {
//			parentT->childrenBegin = parentT->childrenEnd = -1;
//		}
//		_T->nextSibling = _T->prevSibling = -1;
//		_T->parent = 0;

		this->parent->children.erase(childId);
		this->parent = 0;
	}
	void scaleChild(glm::vec3 pos, glm::vec3 scale) {
		_T->position = (_T->position - pos) * scale + pos;
		_T->scale *= scale;
		for (Transform* a : children)
			a->scaleChild(pos, scale);
	}

};
