
#pragma once
#include <glm/glm.hpp>
#include "Component.h"
#include "fast_list.h"
#include "Transform.h"
#include <iostream>
//#include <mutex>

#include <list>
#include <atomic>
#include <deque>
#include "game_object.h"
#include <array>
#include "Input.h"
#include <math.h>
#include <functional>
#include "terrain.h"
#include "collision.h"
using namespace std;


bool testCollision(collider &c1, collider &c2, glm::vec3 &result);

void assignRigidBody(collider *c, rigidBody *rb);

class rigidBody : public component
{
	friend collider;

public:
	bool gravity = true;
	float bounciness = .5f;
	rigidBody();
	bool _registerEngineComponent();

	void onStart();
	void update();
	void onEdit();
	//UPDATE(rigidBody, update);
	COPY(rigidBody);

	int id;
	float mass = 1;
	float damping = 0;
	void setVelocity(glm::vec3 _vel);
	void accelerate(glm::vec3 acc);
	glm::vec3 getVelocity();
	static void collide(collider &a, collider &b, int &colCount);

private:
	glm::vec3 vel = glm::vec3(0);
	glm::quat axis;
	float rotVel;
	SER4(gravity, bounciness, mass, damping);
};

enum colType
{
	aabbType,
	obbType,
	meshType,
	pointType
};

class collider : public component
{
public:
	int layer;
	colType type = obbType;
	// colDat *posInTree = 0;
	glm::vec3 r = glm::vec3(1);
	// colDat cd;
	glm::vec3 dim = vec3(1);
	AABB2 a;
	union
	{
		OBB o;
		mesh m;
		point p;
	};
	bool valid;
	bool collider_shape_updated;

	rigidBody *rb = 0;
	bool _registerEngineComponent();
	void setLayer(int l);
	void onStart();

	void onDestroy();

	void setMesh(Mesh *_m);
	void setPoint();
	void setOBB();
	void update();
	void midUpdate();
	void lateUpdate();
	void update_data();
	void onEdit();
	COPY(collider);

	SER_HELPER(){
		ar &boost::serialization::base_object<component>(*this);
		ar &layer &type &r &dim;
		switch (this->type)
		{
		case aabbType:
			ar & this->a;
			break;
		case obbType:
			ar & this->o;
			break;
		case meshType:
			ar & this->m;
			break;
		case pointType:
			ar & this->p;
			break;
		default:
			break;
		}
	}
};

struct treenode
{
	int axis;
	float d;
	float farthest;

	int id;
	int children;

	bool left;
	bool isLeaf;
	// int objCounter;
	deque<octDat> objs;
	// colDat objs[maxObj];
	void clear();
	void init(bool _left, int _axis, int _parent, int _id);
	void push_back(AABB2 &a, collider *cd);
	mutex m;
	treenode();
	treenode(const treenode &t);
};

struct octree
{
	mutex lock;
	deque<treenode> nodes;

	void clear();
	void query(collider &myCol, int &colCount, treenode &curr);
	void query(collider &myCol, int &colCount);
	int split();
	void insert(AABB2 &a, collider &colliderData, int depth, treenode &curr);
	void insert(AABB2 &a, collider &colliderData, int depth);
};

namespace physics_manager{
	extern map<int, octree> collisionLayers;
	extern map<int, set<int>> collisionGraph;
	extern tbb::concurrent_unordered_map<Mesh*,MESH> meshes;
}
// octree* Octree = new octree();


// float size(glm::vec3 a)
// {
// 	return a.x * a.y * a.z;
// }

void assignRigidBody(collider *c, rigidBody *rb);

bool testCollision(collider &c1, collider &c2, glm::vec3 &result);

bool raycast(vec3 p, vec3 dir);