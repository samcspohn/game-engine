
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
#include "terrain.h"
using namespace std;

const int maxObj = 32;
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

struct colDat
{
	collider *c;
	AABB a;
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
	//	rigidBody(const rigidBody& rb){
	//		mass = rb.mass;
	//		damping = rb.damping;
	//		axis = rb.axis;
	//		rotVel = rb.rotVel;
	//	}
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
	UPDATE(rigidBody, update);
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

void rigidBody::collide(colDat &a, colDat &b, int &colCount)
{

	if (a.c >= b.c)
		return;
	if (a.valid && b.valid && testAABB(a.a, b.a))
	{
		//        cout << "collision";
		glm::vec3 aMin = a.a.c - a.a.r;
		glm::vec3 aMax = a.a.c + a.a.r;

		glm::vec3 bMin = b.a.c - b.a.r;
		glm::vec3 bMax = b.a.c + b.a.r;

		float x;
		if (a.a.c.x < b.a.c.x)
		{
			x = bMin.x - aMax.x;
			if (x > 0)
				x = 0;
		}
		else
		{
			x = bMax.x - aMin.x;
			if (x < 0)
				x = 0;
		}

		float y;
		if (a.a.c.y < b.a.c.y)
		{
			y = bMin.y - aMax.y;
			if (y > 0)
				y = 0;
		}
		else
		{
			y = bMax.y - aMin.y;
			if (y < 0)
				y = 0;
		}

		float z;
		if (a.a.c.z < b.a.c.z)
		{
			z = bMin.z - aMax.z;
			if (z > 0)
				z = 0;
		}
		else
		{
			z = bMax.z - aMin.z;
			if (z < 0)
				z = 0;
		}

		if (abs(x) <= abs(y))
		{
			y = 0;
			if (abs(x) <= abs(z))
				z = 0;
			else
				x = 0;
		}
		else
		{
			x = 0;
			if (abs(y) <= abs(z))
				z = 0;
			else
				y = 0;
		}

		// float x,y,z;
		// if(a.a.c.x < b.a.c.x)
		// 	x = aMax.x - bMin.x;
		// else
		// 	x = aMin.x - bMax.x;

		// if(a.a.c.y < b.a.c.y)
		// 	y = aMax.y - bMin.y;
		// else
		// 	y = aMin.y - bMax.y;

		// if(a.a.c.z < b.a.c.z)
		// 	z = aMax.z - bMin.z;
		// else
		// 	z = aMin.z - bMax.z;

		// x = -x;
		// y = -y;
		// z = -z;
		glm::vec3 r = glm::normalize(a.a.c - b.a.c);
		r *= glm::length(a.a.c - b.a.c) - (a.a.r.x + b.a.r.x);
		r /= Time.deltaTime;
		r /= 2;
		if (vecIsNan(r))
			r = glm::vec3();
		colCount++;

		if (a.rb != 0 && b.rb != 0)
		{
			if (b.rb->mass == 0 || a.rb->mass == 0)
				return;
			float mRatio = b.rb->mass / (b.rb->mass + a.rb->mass);
			//            cout << "collision" << endl;
			//todo find why b is nan
			if (vecIsNan(b.rb->getVelocity()) || vecIsNan(a.rb->getVelocity()))
			{
				cout << "vec is nan" << endl;
				return;
			}
			/*glm::vec3 bvel = b.rb->getVelocity();
			b.rb->setVelocity(a.rb->getVelocity());
			a.rb->setVelocity(bvel);*/

			glm::vec3 aCurr = a.rb->vel;
			a.rb->vel += b.rb->vel - aCurr + glm::vec3(x, y, z) * 2.f; // * 2.f) / a.rb->mass * b.rb->mass;
			b.rb->vel += aCurr - b.rb->vel - glm::vec3(x, y, z) * 2.f; // * 2.f) / b.rb->mass * a.rb->mass;

			if (a.a.c == b.a.c)
			{
				// a.rb->vel += length(glm::vec3(x, y, z)) * randomSphere() * (1 - mRatio);
				// b.rb->vel += length(glm::vec3(x, y, z)) * randomSphere() * (mRatio);
				((component *)a.c)->transform->move(length(glm::vec3(x, y, z)) * randomSphere() * (1 - mRatio)* (1.f/colCount));
				((component *)b.c)->transform->move(-length(glm::vec3(x, y, z)) * randomSphere() * mRatio * (1.f/colCount));
			}
			else
			{
				// a.rb->vel += glm::vec3(x, y, z) * (1 - mRatio);
				// b.rb->vel += -glm::vec3(x, y, z) * mRatio;
				((component *)a.c)->transform->move(glm::vec3(x, y, z) * (1 - mRatio) * (1.f/colCount));
				((component *)b.c)->transform->move(-glm::vec3(x, y, z) * mRatio * (1.f/colCount));
			}
		}
		else if (b.rb != 0 && a.rb == 0)
		{
			//*b.rb->vel = glm::vec3(0);
			//*b.rb->vel -= glm::vec3(x, y, z) * 2.f;
			b.rb->vel = glm::vec3((x == 0 ? b.rb->vel.x : -b.rb->vel.x * 0.5f), (y == 0 ? b.rb->vel.y : -b.rb->vel.y * 0.5f), (z == 0 ? b.rb->vel.z : -b.rb->vel.z * 0.5f)); // glm::vec3(x, y, z);

			((component *)b.c)->transform->move(-glm::vec3(x, y, z));
		}
		else if (a.rb != 0 && b.rb == 0)
		{
			a.rb->vel = glm::vec3((x == 0 ? a.rb->vel.x : -a.rb->vel.x * 0.5f), (y == 0 ? a.rb->vel.y : -a.rb->vel.y * 0.5f), (z == 0 ? a.rb->vel.z : -a.rb->vel.z * 0.5f)); // glm::vec3(x, y, z);
			//*a.rb->vel += glm::vec3(x, y, z) * 2.f;
			((component *)a.c)->transform->move(glm::vec3(x, y, z));
		}
		((component *)a.c)->transform->gameObject->collide(((component *)b.c)->transform->gameObject);
		((component *)b.c)->transform->gameObject->collide(((component *)a.c)->transform->gameObject);
	}
}

atomic<int> octree2Size(0);
struct treenode;
vector<treenode> nodes(1000000);
atomic<int> numNodes;
//mutex nodeLock;
fast_list<treenode *> leaves;
mutex leaveLock;
struct treenode
{
	int axis = 0;
	float d = 0;
	float farthest = 0;

	int id = -1;
	int parent = -1;
	int children = -1;

	bool left;
	bool isLeaf;
	int objCounter = 0;
	colDat objs[maxObj];
	void clear()
	{
		isLeaf = (true);
		objCounter = 0;
	}
	void init(bool _left, int _axis, int _parent, int _id)
	{
		isLeaf =(true);
		objCounter = 0;
		axis = _axis;
		parent = _parent;
		id = _id;
		farthest = 0;
		d = 0;
		children = -1;
		left = _left;
	}
	colDat *push_back(const colDat &cd)
	{
		if (objCounter == maxObj)
			return 0;
		objs[objCounter] = cd;
		return &objs[objCounter++];
	}
	mutex m;

	treenode() : m()
	{
	}
	treenode(const treenode &_t) : m()
	{
	}

	void insert(colDat colliderData, int depth)
	{
		if (!isLeaf)
		{
			if (colliderData.a.c[axis] + colliderData.a.r[axis] < d)
			{
				nodes[children].insert(colliderData, depth + 1);
				// return;
			}
			else if (colliderData.a.c[axis] - colliderData.a.r[axis] > d)
			{
				nodes[children + 1].insert(colliderData, depth + 1);
				// return;
			}
			else
			{
				m.lock();
				float e = _max(abs(colliderData.a.c[axis] - colliderData.a.r[axis]), abs(colliderData.a.c[axis] + colliderData.a.r[axis])) - d;
				if (e > farthest)
					farthest = e;
				setPosInTree(colliderData.c, push_back(colliderData));
				m.unlock();
				// return;
			}
		}
		else
		{
			m.lock();
			if (isLeaf)
			{
				if (objCounter < maxObj)
				{
					setPosInTree(colliderData.c, push_back(colliderData));
				}
				if (objCounter >= maxObj)
				{
					octree2Size.fetch_add(2);
					children = numNodes.fetch_add(2);
					nodes[children].init(true, (axis + 1) % 3, id, children);
					nodes[children + 1].init(false, (axis + 1) % 3, id, children + 1);

					for (auto &i : objs)
					{
						d += i.a.c[axis];
					}
					d /= objCounter;

					farthest = 0;
					int j = 0;
					for (int i = 0; i < objCounter; i++)
					{

						if (objs[i].a.c[axis] + objs[i].a.r[axis] < d)
						{
							// nodes[children].insert(objs[i], 0);
							setPosInTree(objs[i].c, nodes[children].push_back(objs[i]));
						}
						else if (objs[i].a.c[axis] - objs[i].a.r[axis] > d)
						{
							// nodes[children + 1].insert(objs[i],0);
							setPosInTree(objs[i].c, nodes[children + 1].push_back(objs[i]));
						}
						else
						{
							float e = _max(abs(objs[i].a.c[axis] - objs[i].a.r[axis]), abs(objs[i].a.c[axis] + objs[i].a.r[axis])) - d;
							if (e > farthest)
							{
								farthest = e;
							}
							objs[j] = objs[i];
							setPosInTree(objs[j].c, &objs[j]);
							j++;
						}
					}
					objCounter = j;
					isLeaf = (false);
					// m.unlock();
					// return;
				}
			}
			else
			{
				if (colliderData.a.c[axis] + colliderData.a.r[axis] < d)
				{
					// m.unlock();
					nodes[children].insert(colliderData, depth + 1);
					// return;
				}
				else if (colliderData.a.c[axis] - colliderData.a.r[axis] > d)
				{
					// m.unlock();
					nodes[children + 1].insert(colliderData, depth + 1);
					// return;
				}
				else
				{
					float e = _max(abs(colliderData.a.c[axis] - colliderData.a.r[axis]), abs(colliderData.a.c[axis] + colliderData.a.r[axis])) - d;
					if (e > farthest)
						farthest = e;
					setPosInTree(colliderData.c, push_back(colliderData));
					// m.unlock();
					// return;
				}
			}
			m.unlock();

		}
	}

	struct iterator
	{
		int nodeId = 0;
		int objId = 0;
	};

	colDat iterate(AABB &_a, treenode::iterator &itr)
	{
	}

	void query(AABB &_a, colDat &myCol, int &colCount)
	{

		if (isLeaf || _a.c[axis] - _a.r[axis] < d + farthest || _a.c[axis] + _a.r[axis] < d - farthest)
		{
			if (left)
				for (int i = 0; i < objCounter; i++)
				{
					if (_a.c[axis] - _a.r[axis] < objs[i].a.c[axis] + objs[i].a.r[axis])
						rigidBody::collide(myCol, objs[i], colCount);
				}
			else
				for (int i = 0; i < objCounter; i++)
				{
					if (_a.c[axis] + _a.r[axis] > objs[i].a.c[axis] - objs[i].a.r[axis])
						rigidBody::collide(myCol, objs[i], colCount);
				}
		}

		if (isLeaf)
			return;

		if (_a.c[axis] + _a.r[axis] > d && _a.c[axis] - _a.r[axis] < d)
		{
			nodes[children].query(_a, myCol, colCount);
			nodes[children + 1].query(_a, myCol, colCount);
		}
		else if (_a.c[axis] + _a.r[axis] < d)
			nodes[children].query(_a, myCol, colCount);
		else if (_a.c[axis] - _a.r[axis] > d)
			nodes[children + 1].query(_a, myCol, colCount);
	}
};

treenode *octree2;

int colid = 0;
float size(glm::vec3 a)
{
	return a.x * a.y * a.z;
}

struct PosCol
{
	colDat a;
	colDat b;
	PosCol(){};
	PosCol(colDat _a, colDat _b) : a(_a), b(_b){};
};

vector<vector<colDat>> posCol(concurrency::numThreads);

class collider : public component
{
	list<collider *>::iterator itr;

public:
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

	void onDestroy()
	{
		colM.lock();
		if (posInTree)
			posInTree->valid = false;
		collider_manager.colliders.erase(itr);
		colM.unlock();
	}
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
	void update()
	{
		//octLock.lock();
		//if (id == 0) {
		//	cout << "\ncreate tree\n";
		//}
		posInTree = 0;
		a = AABB(transform->getPosition(), r * transform->getScale());
		cd.a = a;
		cd.c = this;
		cd.valid = true;
		cd.rb = rb;
		int depth = 0;
		// leaveLock.lock();
		// octree2->insert(cd, depth);
		// leaveLock.unlock();

		//octLock.unlock();
	}

	rigidBody *rb = 0;
	int id;
	void lateUpdate()
	{

		int colCount = 0;
		// octree2->query(cd.a, cd, colCount);
		for (auto &i : terrains)
		{
			terrainHit h = i.second->getHeight(cd.a.c.x, cd.a.c.z);
			if ((cd.a.c - cd.a.r).y < h.height)
			{
				if (rb != 0)
				{
					rb->vel = glm::reflect(rb->vel,h.normal) * rb->bounciness;
					// rb->vel.y = rb->vel.y >= 0 ? rb->vel.y : -rb->vel.y;
					glm::vec3 p = transform->getPosition();
					transform->setPosition(glm::vec3(p.x, h.height + cd.a.r.y + 0.1f, p.z));
				}
				transform->gameObject->collide(i.second->transform->gameObject);
			}
		}
	}

	UPDATE(collider, update);
	LATE_UPDATE(collider, lateUpdate);

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
