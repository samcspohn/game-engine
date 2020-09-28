
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

class rigidBody;
class collider;
class physicsWorker;

void assignRigidBody(collider *c, rigidBody *rb);

struct colDat
{
	collider *c;
	bool collider_shape_updated;
	AABB2 a;
	OBB o;
	bool valid;
	rigidBody *rb;
	colDat(){};
	colDat(collider *_c, AABB2 _a) : c(_c), a(_a) {}
	void update();
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

mutex wtfboiiiiii;
void rigidBody::collide(colDat &a, colDat &b, int &colCount)
{

	// if (a.c >= b.c)
	// 	return;
	if (a.valid && b.valid && testAABB(a.a, b.a) &&
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
			 return TestOBBOBB(a.o,b.o);
			}()
		//  TestOBBOBB(a.o,b.o)
	 )
	 {
		 ((component *)a.c)->transform->gameObject->collide(((component *)b.c)->transform->gameObject, vec3(0), vec3(0));
		 ((component *)b.c)->transform->gameObject->collide(((component *)a.c)->transform->gameObject, vec3(0), vec3(0));
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

struct octree{
		mutex lock;
		deque<treenode> nodes;

		void clear()
		{
			nodes.clear();
			nodes.push_back(treenode());
			nodes[0].init(0, 0, 0, 0);
		}

		void query(AABB2 & _a, colDat & myCol, int &colCount, treenode &curr)
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
		void query(AABB2 & _a, colDat & myCol, int &colCount)
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
		colDat *posInTree = 0;
		COPY(collider);
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
			id = colid++;
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
			// colM.lock();
			if (posInTree)
				posInTree->valid = false;
			// collider_manager.colliders.erase(itr);
			// colM.unlock();
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
			union
			{
				physics::box box;
				physics::sphere sphere;
				physics::plane plane;
			};
		} _collider;
		// AABB2 a;
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
			glm::vec3 r = vec3(1) * length(sc) * 1.5f;
			cd.a = AABB2(transform->getPosition() - r, transform->getPosition() + r);

			cd.c = this;
			cd.valid = true;
			cd.rb = rb;
			cd.collider_shape_updated = false;

			// cd.o.c = a.getCenter();
			// cd.o.u[0] = rot * glm::vec3(1, 0, 0);
			// cd.o.u[1] = rot * glm::vec3(0, 1, 0);
			// cd.o.u[2] = rot * glm::vec3(0, 0, 1);
			// cd.o.e = sc;

			
			int depth = 0;
			// physLock.lock();
			if (layer == 1)
				collisionLayers[layer].insert(cd, depth);
			// physLock.unlock();

			//octLock.unlock();
		}

		rigidBody *rb = 0;
		int id;
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
					transform->gameObject->collide(t->transform->gameObject, glm::vec3(p.x, h.height, p.z), h.normal);
				}
			}
			// }
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

void colDat::update()
{
	glm::vec3 sc = c->transform->getScale() * c->dim;
	glm::mat3 rot = glm::toMat3(c->transform->getRotation());

	o.c = a.getCenter();
	o.u[0] = rot * glm::vec3(1, 0, 0);
	o.u[1] = rot * glm::vec3(0, 1, 0);
	o.u[2] = rot * glm::vec3(0, 0, 1);
	o.e = sc;
}