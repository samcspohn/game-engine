#pragma once

#include "array_heap.h"
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <mutex>
// #include "Component.h"
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
// extern deque_heap<_transform> TRANSFORMS;
extern vector<_transform> TRANSFORMS_TO_BUFFER;
extern gpu_vector_proxy<_transform>* GPU_TRANSFORMS;
extern gpu_vector_proxy<GLint>* transformIds;
// extern gpu_vector_proxy<_transform>* GPU_TRANSFORMS_UPDATES;
extern gpu_vector_proxy<glm::vec3>* gpu_position_updates;
extern gpu_vector_proxy<glm::quat>* gpu_rotation_updates;
extern gpu_vector_proxy<glm::vec3>* gpu_scale_updates;


class game_object;

struct transform2 {
	int id;
	void _init();
	transform2();
	transform2(int i);
	void init(game_object* g);
	void init(transform2 other, game_object* go);
	void init(transform2 & other);

	_transform getTransform();

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
	list<transform2>& getChildren();
	transform2 getParent();
	void adopt(transform2 transform);
	game_object* gameObject();
	void setGameObject(game_object* g);

	void _destroy();
	
	void move(glm::vec3 movement, bool hasChildren = false);
	void translate(glm::vec3 translation);
	void translate(glm::vec3 translation, glm::quat r);
	void scale(glm::vec3 scale);
	void scaleChild(glm::vec3 pos, glm::vec3 scale);
    void rotate(glm::vec3 axis, float radians);
	void rotateChild(glm::vec3 axis, glm::vec3 pos, glm::quat r, float angle);
	~transform2();
	inline transform2* operator->(){
		return this;
	}

	private:
		friend void destroyRoot(transform2 * t);

	void orphan();
};

struct trans_update {
	bool pos;
	bool rot;
	bool scl;
};

struct transform_meta{
	game_object* gameObject;
	transform2 parent;
	list<transform2> children;
	list<transform2>::iterator childId;
	tbb::spin_mutex m;
};

struct _Transforms {

	tbb::concurrent_vector<glm::vec3> positions;
	tbb::concurrent_vector<glm::quat> rotations;
	tbb::concurrent_vector<glm::vec3> scales;
	tbb::concurrent_vector<transform_meta> meta;
	tbb::concurrent_vector<trans_update> transform_updates;

	// deque<glm::vec3> positions;
	// deque<glm::quat> rotations;
	// deque<glm::vec3> scales;
	// deque<transform_meta> meta;
	// deque<trans_update> transform_updates;
	// tbb::concurrent_priority_queue<int> avail2;
	tbb::concurrent_priority_queue<int> avail;
	tbb::spin_mutex m;

	transform2 _new(){
		transform2 t;
		if(!avail.try_pop(t.id)){
			m.lock();
			if(avail.size() == 0){
				t.id = positions.size();
				m.unlock();

				positions.emplace_back();
				rotations.emplace_back();
				scales.emplace_back();
				meta.emplace_back();
				transform_updates.emplace_back();
			}else{
				avail.try_pop(t.id);
				m.unlock();
			}
		}
		positions[t.id] = glm::vec3(0,0,0);
		rotations[t.id] =  glm::quat(1,0,0,0);
		scales[t.id] = glm::vec3(1,1,1);
		new(&meta[t.id]) transform_meta();
		transform_updates[t.id].pos = true;
		transform_updates[t.id].rot = true;
		transform_updates[t.id].scl = true;

		return t;
	}

	void _delete(transform2 t){
		avail.push(t.id);
	}

	int size(){
		return meta.size();
	}
	float density(){
		return 1.f - avail.size() / meta.size();
	}

	int active(){
		return meta.size() - avail.size();
	}
};

extern _Transforms Transforms;
void initTransform();

int switchAH(int index);
extern unsigned int transforms_enabled;

// extern Transform* root;

extern transform2 root2;







class Transform {
	// mutex m;
	tbb::spin_mutex m;
public:
	deque_heap<_transform>::ref _T;
	game_object* _gameObject;

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
	void adopt(Transform * transform);
	game_object* gameObject();
	void setGameObject(game_object* g);

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