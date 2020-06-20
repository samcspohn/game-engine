
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
using namespace std;

const int maxObj = 128;
const int maxDepth = 100;

atomic<int> rbId;

class rigidBody;
class collider;
class physicsWorker;

void assignRigidBody(collider *c, rigidBody *rb);

namespace physics
{

struct sphere
{
	float radius;
};
struct box
{
	box operator=(glm::vec3 v)
	{
		this->dimensions = v;
		return *this;
	}
	glm::vec3 dimensions;
};
struct plane
{
	glm::vec4 _plane;
};
} // namespace physics

class
{
public:
	list<collider *> colliders;
	//friend collider;
} collider_manager;
mutex colM;

struct AABB
{
	glm::vec3 c;
	glm::vec3 r;
	AABB(glm::vec3 _c, glm::vec3 _r) : c(_c), r(_r){};
	AABB(){};
};
struct OBB {
    glm::vec3 c;     // OBB center point
    glm::vec3 u[3]; // Local x-, y-, and z-axes
    glm::vec3 e;    // Positive halfwidth extents of OBB along each axis
};
bool _testAABB(const AABB &a, const AABB &b)
{
	bool x = abs(a.c[0] - b.c[0]) < (a.r[0] + b.r[0]);
	bool y = abs(a.c[1] - b.c[1]) < (a.r[1] + b.r[1]);
	bool z = abs(a.c[2] - b.c[2]) < (a.r[2] + b.r[2]);

	return x && y && z;
}

bool testAABB(const AABB &a, const AABB &b)
{
	if (abs(a.c[0] - b.c[0]) >= (a.r[0] + b.r[0]))
		return false;
	if (abs(a.c[1] - b.c[1]) >= (a.r[1] + b.r[1]))
		return false;
	if (abs(a.c[2] - b.c[2]) >= (a.r[2] + b.r[2]))
		return false;

	return true;
}

#define EPSILON 0.00001
int TestOBBOBB(OBB &a, OBB &b)
{
    float ra, rb;
    glm::mat3 R, AbsR;

    // Compute rotation matrix expressing b in a's coordinate frame
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            R[i][j] = glm::dot(a.u[i], b.u[j]);

    // Compute translation vector t
    glm::vec3 t = b.c - a.c;
    // Bring translation into a's coordinate frame
    t = glm::vec3(glm::dot(t, a.u[0]), glm::dot(t, a.u[1]), glm::dot(t, a.u[2]));

    // Compute common subexpressions. Add in an epsilon term to
    // counteract arithmetic errors when two edges are parallel and
    // their cross product is (near) null (see text for details)
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            AbsR[i][j] = glm::abs(R[i][j]) + EPSILON;

    // Test axes L = A0, L = A1, L = A2
    for (int i = 0; i < 3; i++) {
        ra = a.e[i];
        rb = b.e[0] * AbsR[i][0] + b.e[1] * AbsR[i][1] + b.e[2] * AbsR[i][2];
        if (glm::abs(t[i]) > ra + rb) return 0;
    }

    // Test axes L = B0, L = B1, L = B2
    for (int i = 0; i < 3; i++) {
        ra = a.e[0] * AbsR[0][i] + a.e[1] * AbsR[1][i] + a.e[2] * AbsR[2][i];
        rb = b.e[i];
        if (glm::abs(t[0] * R[0][i] + t[1] * R[1][i] + t[2] * R[2][i]) > ra + rb) return 0;
    }

    // Test axis L = A0 x B0
    ra = a.e[1] * AbsR[2][0] + a.e[2] * AbsR[1][0];
    rb = b.e[1] * AbsR[0][2] + b.e[2] * AbsR[0][1];
    if (glm::abs(t[2] * R[1][0] - t[1] * R[2][0]) > ra + rb) return 0;

    // Test axis L = A0 x B1
    ra = a.e[1] * AbsR[2][1] + a.e[2] * AbsR[1][1];
    rb = b.e[0] * AbsR[0][2] + b.e[2] * AbsR[0][0];
    if (glm::abs(t[2] * R[1][1] - t[1] * R[2][1]) > ra + rb) return 0;

    // Test axis L = A0 x B2
    ra = a.e[1] * AbsR[2][2] + a.e[2] * AbsR[1][2];
    rb = b.e[0] * AbsR[0][1] + b.e[1] * AbsR[0][0];
    if (glm::abs(t[2] * R[1][2] - t[1] * R[2][2]) > ra + rb) return 0;

    // Test axis L = A1 x B0
    ra = a.e[0] * AbsR[2][0] + a.e[2] * AbsR[0][0];
    rb = b.e[1] * AbsR[1][2] + b.e[2] * AbsR[1][1];
    if (glm::abs(t[0] * R[2][0] - t[2] * R[0][0]) > ra + rb) return 0;

    // Test axis L = A1 x B1
    ra = a.e[0] * AbsR[2][1] + a.e[2] * AbsR[0][1];
    rb = b.e[0] * AbsR[1][2] + b.e[2] * AbsR[1][0];
    if (glm::abs(t[0] * R[2][1] - t[2] * R[0][1]) > ra + rb) return 0;

    // Test axis L = A1 x B2
    ra = a.e[0] * AbsR[2][2] + a.e[2] * AbsR[0][2];
    rb = b.e[0] * AbsR[1][1] + b.e[1] * AbsR[1][0];
    if (glm::abs(t[0] * R[2][2] - t[2] * R[0][2]) > ra + rb) return 0;

    // Test axis L = A2 x B0
    ra = a.e[0] * AbsR[1][0] + a.e[1] * AbsR[0][0];
    rb = b.e[1] * AbsR[2][2] + b.e[2] * AbsR[2][1];
    if (glm::abs(t[1] * R[0][0] - t[0] * R[1][0]) > ra + rb) return 0;

    // Test axis L = A2 x B1
    ra = a.e[0] * AbsR[1][1] + a.e[1] * AbsR[0][1];
    rb = b.e[0] * AbsR[2][2] + b.e[2] * AbsR[2][0];
    if (glm::abs(t[1] * R[0][1] - t[0] * R[1][1]) > ra + rb) return 0;

    // Test axis L = A2 x B2
    ra = a.e[0] * AbsR[1][2] + a.e[1] * AbsR[0][2];
    rb = b.e[0] * AbsR[2][1] + b.e[1] * AbsR[2][0];
    if (glm::abs(t[1] * R[0][2] - t[0] * R[1][2]) > ra + rb) return 0;

    // Since no separating axis found, the OBBs must be intersecting
    return 1;
}
struct colDat
{
	collider *c;
	AABB a;
	OBB o;
	bool valid;
	rigidBody *rb;
	colDat(){};
	colDat(collider *_c, AABB _a) : c(_c), a(_a) {}
};
void setPosInTree(collider *c, colDat *i);

bool vecIsNan(glm::vec3 v)
{
	return isnan(v.x) || isnan(v.y) || isnan(v.z);
}

class rigidBody : public component
{
	friend collider;
	friend physicsWorker;
public:
	bool gravity = true;
	float bounciness = .5f;
	rigidBody()
	{
	}
	bool _registerEngineComponent()
	{
		return true;
	}

	void onStart()
	{
		id = rbId.fetch_add(1);
		auto _c = transform->gameObject->getComponent<collider>();
		if (_c != 0)
		{
			assignRigidBody(_c, transform->gameObject->getComponent<rigidBody>());
		}
		//        mass = 1;
	}
	void update()
	{
		transform->move(vel * Time.deltaTime);
		if(gravity)
			vel += Time.deltaTime * glm::vec3(0, -9.81f, 0);
	}
	//UPDATE(rigidBody, update);
	COPY(rigidBody);

	int id;
	float mass = 1;
	float damping = 0;
	void setVelocity(glm::vec3 _vel)
	{
		if (vecIsNan(_vel))
		{
			cout << "bad vel" << endl;
			throw;
		}
		//lock.lock();
		vel = _vel;
		//lock.unlock();
	}
	void accelerate(glm::vec3 acc)
	{
		//lock.lock();
		vel += acc;
		//lock.unlock();
	}
	glm::vec3 getVelocity()
	{
		if (vecIsNan(vel))
		{
			vel = glm::vec3(0);
		}
		return vel;
	}
	static void collide(colDat &a, colDat &b, int &colCount);

private:
	glm::vec3 vel = glm::vec3(0);
	glm::quat axis;
	float rotVel;
};

mutex wtfboiiiiii;
void rigidBody::collide(colDat &a, colDat &b, int &colCount)
{

	// if (a.c >= b.c)
	// 	return;
	if (a.valid && b.valid && testAABB(a.a, b.a) && TestOBBOBB(a.o,b.o))
	{
		//        cout << "collision";
		// glm::vec3 aMin = a.a.c - a.a.r;
		// glm::vec3 aMax = a.a.c + a.a.r;

		// glm::vec3 bMin = b.a.c - b.a.r;
		// glm::vec3 bMax = b.a.c + b.a.r;

		// float x;
		// if (a.a.c.x < b.a.c.x)
		// {
		// 	x = bMin.x - aMax.x;
		// 	if (x > 0)
		// 		x = 0;
		// }
		// else
		// {
		// 	x = bMax.x - aMin.x;
		// 	if (x < 0)
		// 		x = 0;
		// }

		// float y;
		// if (a.a.c.y < b.a.c.y)
		// {
		// 	y = bMin.y - aMax.y;
		// 	if (y > 0)
		// 		y = 0;
		// }
		// else
		// {
		// 	y = bMax.y - aMin.y;
		// 	if (y < 0)
		// 		y = 0;
		// }

		// float z;
		// if (a.a.c.z < b.a.c.z)
		// {
		// 	z = bMin.z - aMax.z;
		// 	if (z > 0)
		// 		z = 0;
		// }
		// else
		// {
		// 	z = bMax.z - aMin.z;
		// 	if (z < 0)
		// 		z = 0;
		// }

		// if (abs(x) <= abs(y))
		// {
		// 	y = 0;
		// 	if (abs(x) <= abs(z))
		// 		z = 0;
		// 	else
		// 		x = 0;
		// }
		// else
		// {
		// 	x = 0;
		// 	if (abs(y) <= abs(z))
		// 		z = 0;
		// 	else
		// 		y = 0;
		// }

		// glm::vec3 r = glm::normalize(a.a.c - b.a.c);
		// r *= glm::length(a.a.c - b.a.c) - (a.a.r.x + b.a.r.x);
		// r /= Time.deltaTime;
		// r /= 2;
		// if (vecIsNan(r))
		// 	r = glm::vec3();
		// colCount++;

		// if (a.rb != 0 && b.rb != 0)
		// {
		// 	if (b.rb->mass == 0 || a.rb->mass == 0)
		// 		return;
		// 	float mRatio = b.rb->mass / (b.rb->mass + a.rb->mass);
		// 	//            cout << "collision" << endl;
		// 	//todo find why b is nan
		// 	if (vecIsNan(b.rb->getVelocity()) || vecIsNan(a.rb->getVelocity()))
		// 	{
		// 		std::cout << "vec is nan" << endl;
		// 		return;
		// 	}
		// 	/*glm::vec3 bvel = b.rb->getVelocity();
		// 	b.rb->setVelocity(a.rb->getVelocity());
		// 	a.rb->setVelocity(bvel);*/

		// 	glm::vec3 aCurr = a.rb->vel;
		// 	a.rb->vel += b.rb->vel - aCurr + glm::vec3(x, y, z) * 2.f; // * 2.f) / a.rb->mass * b.rb->mass;
		// 	b.rb->vel += aCurr - b.rb->vel - glm::vec3(x, y, z) * 2.f; // * 2.f) / b.rb->mass * a.rb->mass;

		// 	if (a.a.c == b.a.c)
		// 	{
		// 		// a.rb->vel += length(glm::vec3(x, y, z)) * randomSphere() * (1 - mRatio);
		// 		// b.rb->vel += length(glm::vec3(x, y, z)) * randomSphere() * (mRatio);
		// 		((component *)a.c)->transform->move(length(glm::vec3(x, y, z)) * randomSphere() * (1 - mRatio)* (1.f/colCount));
		// 		((component *)b.c)->transform->move(-length(glm::vec3(x, y, z)) * randomSphere() * mRatio * (1.f/colCount));
		// 	}
		// 	else
		// 	{
		// 		// a.rb->vel += glm::vec3(x, y, z) * (1 - mRatio);
		// 		// b.rb->vel += -glm::vec3(x, y, z) * mRatio;
		// 		((component *)a.c)->transform->move(glm::vec3(x, y, z) * (1 - mRatio) * (1.f/colCount));
		// 		((component *)b.c)->transform->move(-glm::vec3(x, y, z) * mRatio * (1.f/colCount));
		// 	}
		// }
		// else if (b.rb != 0 && a.rb == 0)
		// {
		// 	//*b.rb->vel = glm::vec3(0);
		// 	//*b.rb->vel -= glm::vec3(x, y, z) * 2.f;
		// 	b.rb->vel = glm::vec3((x == 0 ? b.rb->vel.x : -b.rb->vel.x * 0.5f), (y == 0 ? b.rb->vel.y : -b.rb->vel.y * 0.5f), (z == 0 ? b.rb->vel.z : -b.rb->vel.z * 0.5f)); // glm::vec3(x, y, z);

		// 	((component *)b.c)->transform->move(-glm::vec3(x, y, z));
		// }
		// else if (a.rb != 0 && b.rb == 0)
		// {
		// 	a.rb->vel = glm::vec3((x == 0 ? a.rb->vel.x : -a.rb->vel.x * 0.5f), (y == 0 ? a.rb->vel.y : -a.rb->vel.y * 0.5f), (z == 0 ? a.rb->vel.z : -a.rb->vel.z * 0.5f)); // glm::vec3(x, y, z);
		// 	//*a.rb->vel += glm::vec3(x, y, z) * 2.f;
		// 	((component *)a.c)->transform->move(glm::vec3(x, y, z));
		// }
		
		((component *)a.c)->transform->gameObject->collide(((component *)b.c)->transform->gameObject,vec3(0),vec3(0));
		((component *)b.c)->transform->gameObject->collide(((component *)a.c)->transform->gameObject,vec3(0),vec3(0));

	}
}

mutex colDatLock;
vector<colDat> colliderData;

struct treenode
{
	int axis;
	float d;
	float farthest;

	int id;
	int children;

	bool left;
	bool isLeaf;
	int objCounter;
	vector<colDat> objs;
	// colDat objs[maxObj];
	void clear()
	{
		isLeaf = (true);
		objCounter = 0;
		objs.clear();
	}
	void init(bool _left, int _axis, int _parent, int _id)
	{
		isLeaf =(true);
		objCounter = 0;
		axis = _axis;
		id = _id;
		farthest = 0;
		d = 0;
		children = -1;
		left = _left;
	}
	colDat *push_back(const colDat &cd)
	{
		objs.push_back(cd);
		objCounter++;
		return &objs.back();
		// if (objCounter == maxObj)
		// 	return 0;
		// objs[objCounter] = cd;
		// return &objs[objCounter++];
	}
	mutex m;
	treenode() : m() {}
	treenode(const treenode& t) : m() {}

};

struct octree{
	mutex lock;
	deque<treenode> nodes;

	void clear(){
		nodes.clear();
		nodes.push_back(treenode());
		nodes[0].init(0,0,0,0);
	}

	void query(AABB &_a, colDat &myCol, int &colCount, treenode& curr)
	{

		if (curr.isLeaf || _a.c[curr.axis] - _a.r[curr.axis] < curr.d + curr.farthest || _a.c[curr.axis] + _a.r[curr.axis] < curr.d - curr.farthest)
		{
			if (curr.left)
				for (int i = 0; i < curr.objCounter; i++)
				{
					if (_a.c[curr.axis] - _a.r[curr.axis] < curr.objs[i].a.c[curr.axis] + curr.objs[i].a.r[curr.axis])
						rigidBody::collide(myCol, curr.objs[i], colCount);
				}
			else
				for (int i = 0; i < curr.objCounter; i++)
				{
					if (_a.c[curr.axis] + _a.r[curr.axis] > curr.objs[i].a.c[curr.axis] - curr.objs[i].a.r[curr.axis])
						rigidBody::collide(myCol, curr.objs[i], colCount);
				}
		}

		if (curr.isLeaf)
			return;

		if (_a.c[curr.axis] + _a.r[curr.axis] > curr.d && _a.c[curr.axis] - _a.r[curr.axis] < curr.d)
		{
			query(_a, myCol, colCount, this->nodes[curr.children]);
			query(_a, myCol, colCount, this->nodes[curr.children + 1]);
		}
		else if (_a.c[curr.axis] + _a.r[curr.axis] < curr.d)
			query(_a, myCol, colCount, this->nodes[curr.children]);
		else if (_a.c[curr.axis] - _a.r[curr.axis] > curr.d)
			query(_a, myCol, colCount, this->nodes[curr.children + 1]);
	}
	void query(AABB &_a, colDat &myCol, int &colCount)
	{
		query(_a,myCol,colCount,nodes[0]);
	}
	int split(){
		lock.lock();
		int size = nodes.size();
		nodes.push_back(treenode());
		nodes.push_back(treenode());
		lock.unlock();
		return size;
	}
	void insert(colDat colliderData, int depth, treenode& curr){
		// auto curr = [&]()-> treenode& {return this->nodes[currId];};
		if (!curr.isLeaf)
		{
			if (colliderData.a.c[curr.axis] + colliderData.a.r[curr.axis] < curr.d)
			{
				insert(colliderData, depth + 1,nodes[curr.children]);
				// nodes[curr.children].insert(colliderData, depth + 1);
				// return;
			}
			else if (colliderData.a.c[curr.axis] - colliderData.a.r[curr.axis] > curr.d)
			{
				insert(colliderData, depth + 1,nodes[curr.children + 1]);
				// nodes[curr.children + 1].insert(colliderData, depth + 1);
				// return;
			}
			else
			{
				curr.m.lock();
				float e = _max(abs(colliderData.a.c[curr.axis] - colliderData.a.r[curr.axis]), abs(colliderData.a.c[curr.axis] + colliderData.a.r[curr.axis])) - curr.d;
				if (e > curr.farthest)
					curr.farthest = e;
				setPosInTree(colliderData.c, curr.push_back(colliderData));
				curr.m.unlock();
				// return;
			}
		}
		else
		{
			curr.m.lock();
			if (curr.isLeaf)
			{
				if (curr.objCounter < maxObj)
				{
					setPosInTree(colliderData.c, curr.push_back(colliderData));
				}
				if (curr.objCounter >= maxObj)
				{

					curr.children = split();
					nodes[curr.children].init(true, (curr.axis + 1) % 3, curr.id, curr.children);
					nodes[curr.children + 1].init(false, (curr.axis + 1) % 3, curr.id, curr.children + 1);

					for (auto &i : curr.objs)
					{
						curr.d += i.a.c[curr.axis];
					}
					curr.d /= curr.objCounter;

					curr.farthest = 0;
					int j = 0;
					for (int i = 0; i < curr.objCounter; i++)
					{

						if (curr.objs[i].a.c[curr.axis] + curr.objs[i].a.r[curr.axis] < curr.d)
						{
							// nodes[curr.children].insert(curr.objs[i], 0);
							setPosInTree(curr.objs[i].c, nodes[curr.children].push_back(curr.objs[i]));
						}
						else if (curr.objs[i].a.c[curr.axis] - curr.objs[i].a.r[curr.axis] > curr.d)
						{
							// nodes[curr.children + 1].insert(curr.objs[i],0);
							setPosInTree(curr.objs[i].c, nodes[curr.children + 1].push_back(curr.objs[i]));
						}
						else
						{
							float e = _max(abs(curr.objs[i].a.c[curr.axis] - curr.objs[i].a.r[curr.axis]), abs(curr.objs[i].a.c[curr.axis] + curr.objs[i].a.r[curr.axis])) - curr.d;
							if (e > curr.farthest)
							{
								curr.farthest = e;
							}
							curr.objs[j] = curr.objs[i];
							setPosInTree(curr.objs[j].c, &curr.objs[j]);
							j++;
						}
					}
					curr.objs.resize(j);
					curr.objCounter = j;
					curr.isLeaf = (false);
					// m.unlock();
					// return;
				}
			}
			else
			{
				if (colliderData.a.c[curr.axis] + colliderData.a.r[curr.axis] < curr.d)
				{
					// m.unlock();
					insert(colliderData, depth + 1,nodes[curr.children]);
					// return;
				}
				else if (colliderData.a.c[curr.axis] - colliderData.a.r[curr.axis] > curr.d)
				{
					// m.unlock();
					insert(colliderData, depth + 1,nodes[curr.children + 1]);
					// return;
				}
				else
				{
					float e = _max(abs(colliderData.a.c[curr.axis] - colliderData.a.r[curr.axis]), abs(colliderData.a.c[curr.axis] + colliderData.a.r[curr.axis])) - curr.d;
					if (e > curr.farthest)
						curr.farthest = e;
					setPosInTree(colliderData.c, curr.push_back(colliderData));
					// m.unlock();
					// return;
				}
			}
			curr.m.unlock();

		}
	}
	void insert(colDat colliderData, int depth )
	{
		// treenode& curr = nodes[0];
		insert(colliderData, depth,nodes[0]);
	}

};

map<int, octree> collisionLayers;
map<int, set<int>> collisionGraph;
// octree* Octree = new octree();

int colid = 0;
float size(glm::vec3 a)
{
	return a.x * a.y * a.z;
}

mutex physLock;
class collider : public component
{
	list<collider *>::iterator itr;

public:
	int layer;
	colDat *posInTree = 0;
	COPY(collider);
	bool _registerEngineComponent()
	{
		return true;
	}
	void onStart()
	{
		posInTree = 0;
		id = colid++;
		colM.lock();
		collider_manager.colliders.push_back(this);
		itr = (--collider_manager.colliders.end());
		colM.unlock();
		auto _rb = transform->gameObject->getComponent<rigidBody>();
		if (_rb != 0)
		{
			//            cout << "assigning rb in collider" << endl;
			rb = _rb;
		}
		_collider = collider_();
	}

	// void onDestroy()
	// {
	// 	colM.lock();
	// 	if (posInTree)
	// 		posInTree->valid = false;
	// 	collider_manager.colliders.erase(itr);
	// 	colM.unlock();
	// }
	glm::vec3 r = glm::vec3(1);
	struct collider_
	{
		collider_()
		{
			type = 0;
			box = glm::vec3(1);
		}
		int type;
		union {
			physics::box box;
			physics::sphere sphere;
			physics::plane plane;
		};
	} _collider;
	AABB a;
	colDat cd;
	glm::vec3 dim = vec3(1);
	void update()
	{
		//octLock.lock();
		//if (id == 0) {
		//	cout << "\ncreate tree\n";
		//}
		posInTree = 0;
		glm::vec3 sc = transform->getScale() * dim;
		glm::mat3 rot = glm::toMat3(transform->getRotation());
		a = AABB(transform->getPosition(), vec3(1) * length(sc) * 1.5f);
		cd.a = a;

		cd.o.c = a.c;
		cd.o.u[0] = rot * glm::vec3(1,0,0);
		cd.o.u[1] = rot * glm::vec3(0,1,0);
		cd.o.u[2] = rot * glm::vec3(0,0,1);
		cd.o.e = sc;

		cd.c = this;
		cd.valid = true;
		cd.rb = rb;
		int depth = 0;
		// physLock.lock();
		if(layer == 1)
			collisionLayers[layer].insert(cd, depth);
		// physLock.unlock();

		//octLock.unlock();
	}

	rigidBody *rb = 0;
	int id;
	void lateUpdate()
	{

		int colCount = 0;
		for(auto & i : collisionGraph[layer]){
			// if(collisionLayers[i].nodes.size() <= collisionLayers[layer].nodes.size())
				collisionLayers[i].query(cd.a, cd, colCount);
		}
		// Octree->query(cd.a, cd, colCount);
		for (auto &i : terrains)
		{
			terrainHit h = i.second->getHeight(cd.a.c.x, cd.a.c.z);
			if ((cd.a.c - cd.a.r).y < h.height)
			{
				glm::vec3 p = transform->getPosition();
				if (rb != 0)
				{
					rb->vel = glm::reflect(rb->vel,h.normal) * rb->bounciness;
					// rb->vel.y = rb->vel.y >= 0 ? rb->vel.y : -rb->vel.y;
					transform->setPosition(glm::vec3(p.x, h.height + cd.a.r.y + 0.1f, p.z));
				}
				transform->gameObject->collide(i.second->transform->gameObject,glm::vec3(p.x, h.height, p.z),h.normal);
			}
		}
	}

	//UPDATE(collider, update);
	// LATE_UPDATE(collider, lateUpdate);

private:
};

void assignRigidBody(collider *c, rigidBody *rb)
{
	c->rb = rb;
}
void setPosInTree(collider *c, colDat *i)
{
	c->posInTree = i;
}
