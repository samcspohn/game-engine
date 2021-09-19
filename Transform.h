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
#include "serialize.h"
// #include "editor.h"
#include <imgui/imgui.h>
// ~-1 = 0

using namespace std;

extern mutex gameLock;
extern glm::vec3 vec3forward;
extern glm::vec3 vec3up;
extern glm::vec3 vec3right;

struct _transform
{
	glm::vec3 position;
	GLint id = -1;
	glm::vec3 scale = glm::vec3(1);
	GLint parent = 0;
	glm::quat rotation = glm::quat(1, 0, 0, 0);
};
extern atomic<int> GPU_TRANSFORMS_UPDATES_itr;
// extern deque_heap<_transform> TRANSFORMS;
extern vector<_transform> TRANSFORMS_TO_BUFFER;
extern gpu_vector_proxy<_transform> *GPU_TRANSFORMS;
extern gpu_vector_proxy<GLint> *transformIds;
// extern gpu_vector_proxy<_transform>* GPU_TRANSFORMS_UPDATES;
extern gpu_vector_proxy<glm::vec3> *gpu_position_updates;
extern gpu_vector_proxy<glm::quat> *gpu_rotation_updates;
extern gpu_vector_proxy<glm::vec3> *gpu_scale_updates;

class game_object;

struct transform2
{
	int id{-1};
	void _init();
	transform2();
	transform2(int i);
	void init(int g);
	void init(transform2 other, int go);
	void init(transform2 &other);

	_transform getTransform();
	string& name();
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
	list<transform2> &getChildren();
	transform2 getParent();
	void adopt(transform2 transform);
	game_object *gameObject();
	void setGameObject(int g);
	// size_t operator=(const transform2& t);
	operator size_t() const { return static_cast<size_t>(id); }

	void _destroy();

	void move(glm::vec3 movement, bool hasChildren = false);
	void translate(glm::vec3 translation);
	void translate(glm::vec3 translation, glm::quat r);
	void scale(glm::vec3 scale);
	void scaleChild(glm::vec3 pos, glm::vec3 scale);
	void rotate(glm::vec3 axis, float radians);
	void rotateChild(glm::vec3 axis, glm::vec3 pos, glm::quat r, float angle);
	~transform2();
	inline transform2 *operator->()
	{
		return this;
	}

private:
	friend void destroyRoot(transform2 *t);

	void orphan();

	// friend OARCHIVE  & operator<<(OARCHIVE  &os, const transform2 &t);
	friend class boost::serialization::access;

	template <class Archive>
	void serialize(Archive &ar, const unsigned int /* file_version */)
	{
		ar &id;
	}
};
// OARCHIVE & operator<<(OARCHIVE  &os, const transform2 &t){
// 	return os << t.id;
// }

struct trans_update
{
	bool pos = true;
	bool rot = true;
	bool scl = true;
	SER_HELPER(){	}
};

struct transform_meta
{
	atomic<int> gameObject;
	transform2 parent{-1};
	list<transform2> children;
	list<transform2>::iterator childId;
	string name;
	// tbb::spin_mutex m;
	mutex m;
	SER_HELPER(){
		ar & parent & name;
	}
};

struct my_spin_lock {
    
    std::atomic_flag flag;

    void lock() {
		for(;;){


			
		}
        while(!flag.test_and_set(std::memory_order_acquire)){

		}
    }
    void unlock() {
        flag.clear(std::memory_order_release);  ;
    }
};


struct _Transforms
{

	std::deque<glm::vec3> positions;
	std::deque<glm::quat> rotations;
	std::deque<glm::vec3> scales;
	std::deque<transform_meta> meta;
	std::deque<trans_update> updates;

	// deque<glm::vec3> positions;
	// deque<glm::quat> rotations;
	// deque<glm::vec3> scales;
	// deque<transform_meta> meta;
	// deque<trans_update> transform_updates;
	// tbb::concurrent_priority_queue<int> avail2;
	std::priority_queue<int,deque<int>,std::greater<int>> avail;
	// tbb::concurrent_priority_queue<int,std::greater<int>> avail;
	// std::queue<int> avail;
	// tbb::spin_mutex m;
	mutex m;



	// transform2 _new(){
	// 	transform2 t;
	// 	if(!avail.try_pop(t.id)){
	// 		m.lock();
	// 		if(avail.size() == 0){
	// 			t.id = positions.size();
	// 			positions.emplace_back();
	// 			rotations.emplace_back();
	// 			scales.emplace_back();
	// 			meta.emplace_back();
	// 			transform_updates.emplace_back();

	// 			m.unlock();
	// 		}else{
	// 			avail.try_pop(t.id);
	// 			m.unlock();
	// 		}
	// 	}
	// 	positions[t.id] = glm::vec3(0,0,0);
	// 	rotations[t.id] =  glm::quat(1,0,0,0);
	// 	scales[t.id] = glm::vec3(1,1,1);
	// 	new(&meta[t.id]) transform_meta();
	// 	new(&transform_updates[t.id]) trans_update();

	// 	return t;
	// }
	int getCount(){
		return meta.size() - avail.size();
	}
	void clear(){
		positions.clear();
		rotations.clear();
		scales.clear();
		meta.clear();
		updates.clear();

		// positions.resize(1);
		// rotations.resize(1);
		// scales.resize(1);
		// meta.resize(1);
		// updates.resize(1);
		while(!avail.empty())
			avail.pop();
	}
	transform2 _new()
	{
		transform2 t;
		m.lock();
		if (avail.size() == 0)
		{
			t.id = positions.size();
			positions.emplace_back();
			rotations.emplace_back();
			scales.emplace_back();
			meta.emplace_back();
			updates.emplace_back();
		}
		else
		{
			t.id = avail.top();
			// t.id = avail.front();
			avail.pop();
		}
		m.unlock();
		positions[t.id] = glm::vec3(0, 0, 0);
		rotations[t.id] = glm::quat(1, 0, 0, 0);
		scales[t.id] = glm::vec3(1, 1, 1);
		new (&meta[t.id]) transform_meta();
		new (&updates[t.id]) trans_update();
		// transform_updates[t.id].pos = true;
		// transform_updates[t.id].rot = true;
		// transform_updates[t.id].scl = true;

		return t;
	}

	void _delete(transform2 t)
	{
		meta[t.id].parent = -1;
		meta[t.id].gameObject = 0;
		meta[t.id].children.clear();
		meta[t.id].childId = list<transform2>::iterator();
		m.lock();
		avail.push(t.id);
		m.unlock();
	}

	int size()
	{
		return meta.size();
	}
	float density()
	{
		return 1.f - avail.size() / meta.size();
	}

	int active()
	{
		return meta.size() - avail.size();
	}
	friend void loadTransforms();
	SER_HELPER(){
		ar & positions & rotations & scales & meta & updates & avail;
	}
};

void newGameObject(transform2 t);
extern _Transforms Transforms;
void initTransform();

int switchAH(int index);
extern unsigned int transforms_enabled;
void renderEdit(const char* name, transform2& t);
// void saveTransforms(OARCHIVE &oa);
// void loadTransforms(IARCHIVE &ia);

extern transform2 root2;
bool operator<(const transform2 &l, const transform2 &r);
bool operator==(const transform2 &l, const transform2 &r);

extern unordered_map<int,int> transform_map;

namespace YAML
{

	template <>
	struct convert<transform2>
	{
		static Node encode(const transform2 &rhs)
		{
			Node node;
			node = rhs.id;
			return node;
		}

		static bool decode(const Node &node, transform2 &rhs)
		{
			int id = node.as<int>();
			rhs.id = transform_map.at(id);
			return true;
		}
	};
}
