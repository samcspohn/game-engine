
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

const int maxObj = 8;
const int maxDepth = 100;

atomic<int> rbId;

class physicsWorker;
bool testCollision(colDat &c1, colDat &c2, glm::vec3 &result);

void assignRigidBody(collider *c, rigidBody *rb);

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
		auto _c = transform->gameObject()->getComponent<collider>();
		if (_c != 0)
		{
			assignRigidBody(_c, transform->gameObject()->getComponent<rigidBody>());
		}
		//        mass = 1;
	}
	void update()
	{
		transform->move(vel * Time.deltaTime);
		if (gravity)
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

void rigidBody::collide(colDat &a, colDat &b, int &colCount)
{

	// if (a.c >= b.c)
	// 	return;
	glm::vec3 res;
	if (a.c != b.c && a.valid && b.valid && testAABB(a.a, b.a) &&
		[&] {
			if (!a.collider_shape_updated)
			{
				a.update();
				a.collider_shape_updated = true;
			}
			if (!b.collider_shape_updated)
			{
				b.update();
				b.collider_shape_updated = true;
			}
			// return o1.c->collide(o2.c);
			//  return TestOBBOBB(a.o,b.o);
			return testCollision(a, b, res);
		}())
	{
		((component *)a.c)->transform->gameObject()->collide(((component *)b.c)->transform->gameObject(), res, vec3(0));
		((component *)b.c)->transform->gameObject()->collide(((component *)a.c)->transform->gameObject(), res, vec3(0));
	}
}
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
	deque<colDat> objs;
	// colDat objs[maxObj];
	void clear()
	{
		isLeaf = (true);
		objCounter = 0;
		objs.clear();
	}
	void init(bool _left, int _axis, int _parent, int _id)
	{
		isLeaf = (true);
		objCounter = 0;
		axis = _axis;
		id = _id;
		farthest = 0;
		d = 0;
		children = -1;
		left = _left;
		// m.unlock();
	}
	colDat *push_back(const colDat &cd)
	{
		objs.push_back(cd);
		objCounter++;
		return &objs.back();
	}
	mutex m;
	treenode() : m() {}
	treenode(const treenode &t) : m() {}
};

struct octree
{
	mutex lock;
	deque<treenode> nodes;

	void clear()
	{
		nodes.clear();
		nodes.push_back(treenode());
		nodes[0].init(0, 0, 0, 0);
	}

	void query(AABB2 &_a, colDat &myCol, int &colCount, treenode &curr)
	{

		if (curr.isLeaf || _a.min[curr.axis] < curr.d + curr.farthest || _a.max[curr.axis] < curr.d - curr.farthest)
		{
			if (curr.left)
				for (int i = 0; i < curr.objCounter; i++)
				{
					if (_a.min[curr.axis] < curr.objs[i].a.max[curr.axis])
						rigidBody::collide(myCol, curr.objs[i], colCount);
				}
			else
				for (int i = 0; i < curr.objCounter; i++)
				{
					if (_a.max[curr.axis] > curr.objs[i].a.min[curr.axis])
						rigidBody::collide(myCol, curr.objs[i], colCount);
				}
		}

		if (curr.isLeaf)
			return;

		if (_a.max[curr.axis] > curr.d && _a.min[curr.axis] < curr.d)
		{
			query(_a, myCol, colCount, this->nodes[curr.children]);
			query(_a, myCol, colCount, this->nodes[curr.children + 1]);
		}
		else if (_a.max[curr.axis] < curr.d)
			query(_a, myCol, colCount, this->nodes[curr.children]);
		else if (_a.min[curr.axis] > curr.d)
			query(_a, myCol, colCount, this->nodes[curr.children + 1]);
	}
	void query(AABB2 &_a, colDat &myCol, int &colCount)
	{
		query(_a, myCol, colCount, nodes[0]);
	}
	int split()
	{
		lock.lock();
		int size = nodes.size();
		nodes.push_back(treenode());
		nodes.push_back(treenode());
		lock.unlock();
		return size;
	}
	void insert(colDat colliderData, int depth, treenode &curr)
	{
		// auto curr = [&]()-> treenode& {return this->nodes[currId];};
		if (!curr.isLeaf)
		{
			if (colliderData.a.max[curr.axis] < curr.d)
			{
				insert(colliderData, depth + 1, nodes[curr.children]);
				// nodes[curr.children].insert(colliderData, depth + 1);
				// return;
			}
			else if (colliderData.a.min[curr.axis] > curr.d)
			{
				insert(colliderData, depth + 1, nodes[curr.children + 1]);
				// nodes[curr.children + 1].insert(colliderData, depth + 1);
				// return;
			}
			else
			{
				curr.m.lock();
				float e = _max(abs(colliderData.a.min[curr.axis]), abs(colliderData.a.max[curr.axis])) - curr.d;
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
						curr.d += i.a.getCenter()[curr.axis];
					}
					curr.d /= curr.objCounter;

					curr.farthest = 0;
					int j = 0;
					for (int i = 0; i < curr.objCounter; i++)
					{

						if (curr.objs[i].a.max[curr.axis] < curr.d)
						{
							// nodes[curr.children].insert(curr.objs[i], 0);
							setPosInTree(curr.objs[i].c, nodes[curr.children].push_back(curr.objs[i]));
						}
						else if (curr.objs[i].a.min[curr.axis] > curr.d)
						{
							// nodes[curr.children + 1].insert(curr.objs[i],0);
							setPosInTree(curr.objs[i].c, nodes[curr.children + 1].push_back(curr.objs[i]));
						}
						else
						{
							float e = _max(abs(curr.objs[i].a.min[curr.axis]), abs(curr.objs[i].a.max[curr.axis])) - curr.d;
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
				if (colliderData.a.max[curr.axis] < curr.d)
				{
					// m.unlock();
					insert(colliderData, depth + 1, nodes[curr.children]);
					// return;
				}
				else if (colliderData.a.min[curr.axis] > curr.d)
				{
					// m.unlock();
					insert(colliderData, depth + 1, nodes[curr.children + 1]);
					// return;
				}
				else
				{
					float e = _max(abs(colliderData.a.min[curr.axis]), abs(colliderData.a.max[curr.axis])) - curr.d;
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
	void insert(colDat colliderData, int depth)
	{
		// treenode& curr = nodes[0];
		insert(colliderData, depth, nodes[0]);
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
public:
	int layer;
	int type = 1;
	colDat *posInTree = 0;
	glm::vec3 r = glm::vec3(1);
	colDat cd;
	glm::vec3 dim = vec3(1);
	rigidBody *rb = 0;
	bool _registerEngineComponent()
	{
		return true;
	}
	void setLayer(int l)
	{
		layer = l;
	}
	void onStart()
	{
		posInTree = 0;
		auto _rb = transform->gameObject()->getComponent<rigidBody>();
		if (_rb != 0)
		{
			rb = _rb;
		}
	}
	void onDestroy()
	{
		if (posInTree)
			posInTree->valid = false;
	}

	void setMesh(Mesh &_m)
	{
		cd.type = 2;
		this->type = 2;
		cd.m.points = &_m.vertices;
		cd.m.tris = &_m.indices;
	}
	void setPoint()
	{
		cd.type = 3;
		this->type = 3;
		cd.p = point();
	}
	void update()
	{

		posInTree = 0;
		switch (type)
		{
		case 0:
		case 1:
		case 2:
		{
			glm::vec3 sc = transform->getScale() * dim;
			// glm::mat3 rot = glm::toMat3(transform->getRotation());
			glm::vec3 r = vec3(length(sc));
			glm::vec3 pos = transform->getPosition();
			cd.a = AABB2(pos - r, pos + r);
			cd.collider_shape_updated = false;
		}
		break;
		case 3:
		{
			if(cd.p.pos1 == vec3(0)){
				cd.p.pos2 = transform->getPosition();
			}else{
				cd.p.pos2 = cd.p.pos1;
			}
			cd.p.pos1 = transform->getPosition();
			// cd.a.max = cd.p.pos1;
			// cd.a.min = cd.p.pos2;
			// if(cd.a.max.x < cd.a.min.x)
			// 	std::swap(cd.a.max.x,cd.a.min.x);
			// if(cd.a.max.y < cd.a.min.y)
			// 	std::swap(cd.a.max.y,cd.a.min.y);
			// if(cd.a.max.z < cd.a.min.z)
			// 	std::swap(cd.a.max.z,cd.a.min.z);


			new(&cd.a.max) glm::vec3(glm::max(cd.p.pos1.x, cd.p.pos2.x),
								 glm::max(cd.p.pos1.y, cd.p.pos2.y),
								 glm::max(cd.p.pos1.z, cd.p.pos2.z));

			new(&cd.a.min) glm::vec3(glm::min(cd.p.pos1.x, cd.p.pos2.x),
								 glm::min(cd.p.pos1.y, cd.p.pos2.y),
								 glm::min(cd.p.pos1.z, cd.p.pos2.z));


			// // x
			// if(cd.p.pos1.x > cd.p.pos2.x){
			// 	cd.a.max.x = cd.p.pos1.x;cd.a.min.x = cd.p.pos2.x;
			// }else{
			// 	cd.a.max.x = cd.p.pos2.x;cd.a.min.x = cd.p.pos1.x;
			// }
			// // y
			// if(cd.p.pos1.y > cd.p.pos2.y){
			// 	cd.a.max.y = cd.p.pos1.y;cd.a.min.y = cd.p.pos2.y;
			// }else{
			// 	cd.a.max.y = cd.p.pos2.y;cd.a.min.y = cd.p.pos1.y;
			// }
			// // z
			// if(cd.p.pos1.z > cd.p.pos2.z){
			// 	cd.a.max.z = cd.p.pos1.z;cd.a.min.z = cd.p.pos2.z;
			// }else{
			// 	cd.a.max.z = cd.p.pos2.z;cd.a.min.z = cd.p.pos1.z;
			// }
			cd.collider_shape_updated = true;
		}
		}
		// glm::vec3 sc = transform->getScale() * dim;
		// // glm::mat3 rot = glm::toMat3(transform->getRotation());
		// glm::vec3 r = vec3(length(sc));
		// glm::vec3 pos = transform->getPosition();
		// cd.a = AABB2(pos - r, pos + r);
		cd.type = type;
		cd.c = this;
		cd.valid = true;
		cd.rb = rb;
		// cd.collider_shape_updated = false;

		// int depth = 0;
		// if (layer == 1)
		// 	collisionLayers[layer].insert(cd, depth);
	}
	void midUpdate()
	{
		int depth = 0;
		if (layer == 1)
			collisionLayers[layer].insert(cd, depth);
	}

	void lateUpdate()
	{
		int colCount = 0;
		for (auto &i : collisionGraph[layer])
		{
			// if(collisionLayers[i].nodes.size() <= collisionLayers[layer].nodes.size())
			collisionLayers[i].query(cd.a, cd, colCount);
		}
		// Octree->query(cd.a, cd, colCount);
		// for (auto &i : terrains)
		// {
		// terrainHit h = i.second->getHeight(cd.a.c.x, cd.a.c.z);
		glm::vec3 center = cd.a.getCenter();
		terrain *t = getTerrain(center.x, center.z);
		if (t != 0)
		{
			if (cd.a.min.y > t->max_height * t->transform->getScale().y + t->transform->getPosition().y)
				return;
			terrainHit h = t->getHeight(center.x, center.z);
			if (cd.a.min.y < h.height)
			{
				glm::vec3 p = transform->getPosition();
				if (rb != 0)
				{
					rb->vel = glm::reflect(rb->vel, h.normal) * rb->bounciness;
					// rb->vel.y = rb->vel.y >= 0 ? rb->vel.y : -rb->vel.y;
					transform->setPosition(glm::vec3(p.x, h.height + cd.a.min.y + 0.1f, p.z));
				}
				// transform->gameObject->collide(i.second->transform->gameObject,glm::vec3(p.x, h.height, p.z),h.normal);
				transform->gameObject()->collide(t->transform->gameObject(), glm::vec3(p.x, h.height, p.z), h.normal);
			}
		}
	}

	COPY(collider);

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

void colDat::update()
{
	switch (type)
	{
	case 1: // obb
	{
		glm::vec3 sc = c->transform->getScale() * c->dim;
		glm::mat3 rot = glm::toMat3(c->transform->getRotation());

		o.c = a.getCenter();
		o.u = rot;
		// o.u[0] = rot * glm::vec3(1, 0, 0);
		// o.u[1] = rot * glm::vec3(0, 1, 0);
		// o.u[2] = rot * glm::vec3(0, 0, 1);
		o.e = sc;
	}
	break;
	case 2:
		break;
	case 3: // point
	{
		// p.pos2 = p.pos1;
		// p.pos1 = c->transform->getPosition();
	}
	break;
	default:
		break;
	}
}

bool testCollision(colDat &c1, colDat &c2, glm::vec3 &result)
{

	colDat *a;
	colDat *b;
	mat4 trans;
	mat4 trans2;
	if (c1.type <= c2.type)
	{
		a = &c1;
		b = &c2;
	}
	else
	{
		a = &c2;
		b = &c1;
	}
	switch (a->type)
	{
	case 0: // aabb
		switch (b->type)
		{
		case 0: // aabb
			return true;
			break;
		case 1: //aabb - obb

			break;
		default:
			break;
		}
		break;
	case 1: // obb
		switch (b->type)
		{
		case 1: // obb
			return TestOBBOBB(a->o, b->o);
			break;
		case 2: // mesh
			trans = a->c->transform->getModel();
			trans2 = b->c->transform->getModel();
			return testOBBMesh(a->o, trans, b->m, trans2, result);
			break;
		case 3:
			return testPointOBB(b->p, a->o, result);
		default:
			break;
		}
	case 2: // mesh
		switch (b->type)
		{
		case 2:
			mesh *m1;
			mesh *m2;
			if (a->m.tris->size() <= b->m.tris->size())
			{
				m1 = &a->m;
				m2 = &b->m;
				trans = glm::inverse(a->c->transform->getModel()) * b->c->transform->getModel();
			}
			else
			{
				m1 = &b->m;
				m2 = &a->m;
				trans = glm::inverse(b->c->transform->getModel()) * a->c->transform->getModel();
			}
			return testMeshMesh(*m1, trans, *m2, trans2, result);
			break;
		case 3: // point
			return testPointMesh(b->p, a->m, a->c->transform->getPosition(), a->c->transform->getScale(), a->c->transform->getRotation(), result);
		default:
			break;
		}
	case 3: //point
		switch (b->type)
		{
		// case 0: // aabb
		// 	break;
		// case 1: // obb
		// 	return testPointOBB(b->p,a->o);
		// case 2: // mesh
		// 	return testPointMesh(b->p,a->m,a->c->transform->getPosition(),a->c->transform->getRotation(), result);
		default:
			break;
		}

	default:
		break;
	}
	return false;
}