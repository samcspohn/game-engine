
#pragma once
#include <glm/glm.hpp>
#include "components/Component.h"
#include "fast_list.h"
#include "Transform.h"
#include <iostream>
//#include <mutex>

#include <list>
#include <atomic>
#include <deque>
#include "components/game_object.h"
#include <array>
#include "Input.h"
#include <math.h>
#include <functional>
// #include "terrain.h"
#include "collision.h"
#include "_rendering/Mesh.h"
#include "terrain.h"
using namespace std;

bool testCollision(collider &c1, collider &c2,collision& col);

void assignRigidBody(collider *c, rigidBody *rb);

class rigidBody : public component
{
	friend collider;

public:
	bool gravity = true;
	// float bounciness = .5f;
	rigidBody();
	bool _registerEngineComponent();

	void onStart();
	void _update(float dt);
	void onEdit();
	//UPDATE(rigidBody, update);

	int id;
	float inverse_mass = 1;
	float damping = 0;
	void setVelocity(glm::vec3 _vel);
	void accelerate(glm::vec3 acc);
	glm::vec3 getVelocity();
	SER_FUNC()
	{
		SER(gravity)
		// SER(bounciness)
		SER(inverse_mass)
		SER(damping)
	}

	glm::vec3 vel = glm::vec3(0);
	glm::vec3 ang_vel = glm::vec3(0);
private:
};

enum colType
{
	aabbType,
	obbType,
	meshType,
	pointType,
	sphereType,
	planeType
};

class kinematicBody : public component
{

public:
	static glm::vec3 gravity;
	glm::vec3 velocity;
	void update();
	void onCollision(collision &);
	SER_FUNC(){}
};

namespace YAML
{
	template <>
	struct convert<colType>
	{
		static Node encode(const colType &rhs)
		{
			Node node;
			node["type"] = static_cast<int>(rhs);
			return node;
		}

		static bool decode(const Node &node, colType &rhs)
		{
			rhs = static_cast<colType>(node["type"].as<int>());
			return true;
		}
	};
}

class collider : public component
{
public:
	int layer;
	colType type = obbType;
	// colDat *posInTree = 0;
	glm::vec3 r = glm::vec3(1);
	// colDat cd;
	glm::vec3 dim = glm::vec3(1);
	AABB2 a;
	union
	{
		OBB o;
		mesh m;
		point p;
		Sphere s;
		Plane pl;
	};
	bool valid;
	bool collider_shape_updated;

	rigidBody *rb = 0;
	static void collide(collider &a, collider &b, int &colCount);

	bool _registerEngineComponent();
	void setLayer(int l);
	void onStart();

	void onDestroy();

	glm::vec3 setMesh(Mesh *_m);
	void setPoint();
	void setOBB();
	void setSphere();
	void setPlane();
	void _update();
	void midUpdate();
	// void lateUpdate();
	void _lateUpdate();
	void update_data();
	void ser_edit(ser_mode x, YAML::Node &);
	// collider();
	// collider(const collider&);
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

namespace physics_manager
{
	extern map<int, octree> collisionLayers;
	extern map<int, set<int>> collisionGraph;
	extern tbb::concurrent_unordered_map<Mesh *, MESH> meshes;
}
// octree* Octree = new octree();

// float size(glm::vec3 a)
// {
// 	return a.x * a.y * a.z;
// }

void assignRigidBody(collider *c, rigidBody *rb);

bool testCollision(collider &c1, collider &c2, glm::vec3 &result);

bool raycast(glm::vec3 p, glm::vec3 dir, glm::vec3 &result);

void resolve_collision(collision& col);