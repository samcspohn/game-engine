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
#include "_rendering/gpu_vector.h"
#include "serialize.h"
// #include "editor.h"
#include "imgui/imgui.h"
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
	void init(game_object* g);
	void init(transform2 other, game_object* go);
	void init(transform2 &other);

	_transform getTransform();
	string &name();
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
	void setGameObject(game_object* g);
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
};
// OARCHIVE & operator<<(OARCHIVE  &os, const transform2 &t){
// 	return os << t.id;
// }
void setRotationChild(transform2 tc, glm::quat &rot, glm::vec3 &pos);

struct trans_update
{
	bool pos = true;
	bool rot = true;
	bool scl = true;
};

struct transform_meta
{
	game_object* gameObject = nullptr;
	transform2 parent{-1};
	list<transform2> children;
	list<transform2>::iterator childId;
	string name;
	// tbb::spin_mutex m;
	mutex m;
};

struct my_spin_lock
{

	std::atomic_flag flag;

	void lock()
	{
		for (;;)
		{
		}
		while (!flag.test_and_set(std::memory_order_acquire))
		{
		}
	}
	void unlock()
	{
		flag.clear(std::memory_order_release);
		;
	}
};

struct _Transforms
{
	#define tfm_strg std::deque
	// #define tfm_strg tbb::concurrent_vector
	tfm_strg<glm::vec3> positions;
	tfm_strg<glm::quat> rotations;
	tfm_strg<glm::vec3> scales;
	tfm_strg<transform_meta> meta;
	tfm_strg<trans_update> updates;
	tbb::concurrent_priority_queue<int,std::greater<int>> avail;
	mutex m;

	int getCount()
	{
		return meta.size() - avail.size();
	}

	void clear()
	{
		positions.clear();
		rotations.clear();
		scales.clear();
		meta.clear();
		updates.clear();
		avail.clear();
	}

	transform2 _new()
	{
		transform2 t;
		if(!avail.try_pop(t.id)){
			lock_guard<mutex> lck(m);
			t.id = positions.size();
			// t.id = meta.emplace_back();
			positions.emplace_back();
			rotations.emplace_back();
			scales.emplace_back();
			meta.emplace_back();
			updates.emplace_back();
		}
		positions[t.id] = glm::vec3(0, 0, 0);
		rotations[t.id] = glm::quat(1, 0, 0, 0);
		scales[t.id] = glm::vec3(1, 1, 1);
		new (&meta[t.id]) transform_meta();
		new (&updates[t.id]) trans_update();
		return t;
	}

	void _delete(transform2 t)
	{
		// lock_guard<mutex> lck(m);
		meta[t.id].~transform_meta();
		updates[t.id].~trans_update();
		// meta[t.id].parent = -1;
		// meta[t.id].gameObject = (game_object*)-1;
		// meta[t.id].children.clear();
		// meta[t.id].childId = list<transform2>::iterator();
		// avail[avail_id.fetch_add(1)] = t.id;
		// m.lock();
		avail.push(t.id);
		// m.unlock();
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
	#undef tfm_strg
};

void newGameObject(transform2 t);
extern _Transforms Transforms;
void initTransform();

int switchAH(int index);
extern unsigned int transforms_enabled;
void renderEdit(const char *name, transform2 &t);
// void saveTransforms(OARCHIVE &oa);
// void loadTransforms(IARCHIVE &ia);

extern transform2 root2;
bool operator<(const transform2 &l, const transform2 &r);
bool operator==(const transform2 &l, const transform2 &r);

extern unordered_map<int, int> transform_map;

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
