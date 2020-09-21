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
#include "gpu_vector.h"
// ~-1 = 0

using namespace std;

extern mutex gameLock;
extern glm::vec3 vec3forward;
extern glm::vec3 vec3up;
extern glm::vec3 vec3right;
extern glm::vec3 mainCamPos;
extern glm::vec3 mainCamUp;
extern glm::vec3 MainCamForward;

struct _transform {
	glm::vec3 position; GLint id = -1;
	glm::vec3 scale = glm::vec3(1); GLint parent = 0;
	glm::quat rotation = glm::quat(1,0,0,0);
	void translate(glm::vec3 translation);
	void rotate(glm::vec3 axis, float radians);
};
extern atomic<int> GPU_TRANSFORMS_UPDATES_itr;
extern deque_heap<_transform> TRANSFORMS;
extern vector<_transform> TRANSFORMS_TO_BUFFER;
extern deque_heap<_transform> STATIC_TRANSFORMS;
extern gpu_vector_proxy<_transform>* GPU_TRANSFORMS;
extern gpu_vector_proxy<GLuint>* transformIds;
extern gpu_vector_proxy<_transform>* GPU_TRANSFORMS_UPDATES;

void initTransform();

int switchAH(int index);
extern unsigned int transforms_enabled;
_transform& get_T(int index);

extern Transform* root;

class Transform {
	// mutex m;
	tbb::spin_mutex m;
public:
	deque_heap<_transform>::ref _T;
	game_object* gameObject;

	void init();
	Transform(game_object* g);
	Transform(Transform& other, game_object* go);

	// Transform operator=(const Transform& t);
	void lookat(glm::vec3 lookatPoint, glm::vec3 up);
	glm::vec3 forward();
	glm::vec3 right();
	glm::vec3 up();
	glm::mat4 getModel();
	glm::vec3 getScale();
	void setScale(glm::vec3 scale);
	glm::vec3 getPosition();
	void setPosition(glm::vec3 pos);
	glm::quat getRotation();
	void setRotation(glm::quat r);
	list<Transform*>& getChildren();
	Transform* getParent();
	void Adopt(Transform * transform);

	void _destroy();
	
	void move(glm::vec3 movement, bool hasChildren = false);
	void translate(glm::vec3 translation);
	void translate(glm::vec3 translation, glm::quat r);
	void scale(glm::vec3 scale);
	void scaleChild(glm::vec3 pos, glm::vec3 scale);
    void rotate(glm::vec3 axis, float radians);
	void rotateChild(glm::vec3 axis, glm::vec3 pos, glm::quat r, float angle);

private:
	// bool enabled = true;
	Transform * parent;
	list<Transform*> children;
	list<Transform*>::iterator childId;

	friend void destroyRoot(Transform * t);
	~Transform();
	Transform(Transform & other);

	void orphan();
};

bool compareTransform(Transform* t1, Transform* t2);