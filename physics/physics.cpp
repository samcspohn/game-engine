
#include "physics.h"
using namespace std;

const int maxObj = 1000;
const int maxDepth = 100;

atomic<int> rbId;

bool vecIsNan(glm::vec3 v)
{
	return isnan(v.x) || isnan(v.y) || isnan(v.z);
}

void resolve_collision(collision& col){
	//   glm::vec3 normal = glm::normalize(transform.getPosition() - col.point);
        // glm::vec3 penetration = col.point - col.this_collider->transform->getPosition();
        // float l = glm::length(penetration);
        // penetration = penetration * (1.f - l);
		float penetration = col.penetration;

        // sphere_test *sp = col.g_o->getComponent<sphere_test>();

        float inverse_massB{0};
        if (col.other_collider->rb)
            inverse_massB = col.other_collider->rb->inverse_mass;
        float total_mass = col.this_collider->rb->inverse_mass + inverse_massB;

        // pos
        glm::vec3 relativeA = col.point - col.this_collider->transform->getPosition();
        glm::vec3 relativeB = col.point - col.other_collider->transform->getPosition();

        col.this_collider->rb->transform.move(col.normal * -penetration * (col.this_collider->rb->inverse_mass / total_mass));
		if(col.other_collider->rb)
			col.other_collider->rb->transform.move(col.normal * penetration * (col.other_collider->rb->inverse_mass / total_mass));

        // ang vel
        glm::vec3 angular_velA = glm::cross(col.this_collider->rb->ang_vel, relativeA);
        glm::vec3 angular_velB{0};
        if (col.other_collider->rb)
            angular_velB = glm::cross(col.other_collider->rb->ang_vel, relativeB);
            
        // vel
        glm::vec3 full_velA = col.this_collider->rb->vel + angular_velA;
        glm::vec3 full_velB{0};
        if (col.other_collider->rb)
        {
            full_velB = col.other_collider->rb->vel + angular_velB;
        }

        glm::vec3 contact_vel = full_velB - full_velA;
        float impulse_force = glm::dot(contact_vel, col.normal);

        using namespace glm;
        vec3 inertiaA = cross(cross(relativeA, col.normal), relativeA);
        vec3 inertiaB = cross(cross(relativeB, col.normal), relativeB);
        float ang_effect = dot(inertiaA + inertiaB, col.normal);
        float cRestitution = 0.66f;
        float j = (-(1.0f + cRestitution) * impulse_force) / (total_mass + ang_effect);
        vec3 full_imp = col.normal * j;

        // vec3 force = cross(relativeA, -full_imp);

        col.this_collider->rb->vel += -full_imp * col.this_collider->rb->inverse_mass;
        col.this_collider->rb->ang_vel += cross(relativeA, -full_imp);

		if(col.other_collider->rb){
			col.other_collider->rb->vel += full_imp * col.other_collider->rb->inverse_mass;
			col.other_collider->rb->ang_vel += cross(relativeA, full_imp);
		}
}

rigidBody::rigidBody()
{
}
bool rigidBody::_registerEngineComponent()
{
	return true;
}

void rigidBody::onStart()
{
	id = rbId.fetch_add(1);
	auto _c = transform->gameObject()->getComponent<collider>();
	if (_c != 0)
	{
		assignRigidBody(_c, transform->gameObject()->getComponent<rigidBody>());
	}
	//        mass = 1;
}
void rigidBody::_update(float dt)
{
	  if(inverse_mass != 0){
            vel += glm::vec3(0.f, -10.f, 0.f) * dt;
			transform->move(vel * dt);

			glm::quat rot = transform->getRotation();
			glm::vec3 r = ang_vel * dt;
			rot = rot + glm::quat(r.x, r.y, r.z, 0.f) * rot;
			rot = glm::normalize(rot);
			transform->setRotation(rot);
	  }
}
void rigidBody::onEdit()
{
	RENDER(inverse_mass);
	RENDER(damping)
	// RENDER(bounciness);
	RENDER(gravity);
}
// UPDATE(rigidBody, update);

void rigidBody::setVelocity(glm::vec3 _vel)
{
	if (vecIsNan(_vel))
	{
		cout << "bad vel" << endl;
		throw;
	}
	// lock.lock();
	vel = _vel;
	// lock.unlock();
}
void rigidBody::accelerate(glm::vec3 acc)
{
	// lock.lock();
	vel += acc;
	// lock.unlock();
}
glm::vec3 rigidBody::getVelocity()
{
	if (vecIsNan(vel))
	{
		vel = glm::vec3(0);
	}
	return vel;
}

void collider::collide(collider &a, collider &b, int &colCount)
{

	// if (a.c >= b.c)
	// 	return;
	collision c;
	if (&a < &b && a.valid && b.valid && testAABB(a.a, b.a) &&
		[&]
		{
			if (!a.collider_shape_updated)
			{
				a.update_data();
				a.collider_shape_updated = true;
			}
			if (!b.collider_shape_updated)
			{
				b.update_data();
				b.collider_shape_updated = true;
			}
			return testCollision(a, b, c);
		}())
	{
		c.this_collider = &a;
		c.other_collider = &b;
		c.g_o = a.transform->gameObject();
		// collision c{res, normal, &a, &b, b.transform->gameObject()};
		// a.transform->gameObject()->collide(c);
		if(a.rb != 0)
			resolve_collision(c);

		// c = collision{res, normal, &b, &a, a.transform->gameObject()};
		// b.transform->gameObject()->collide(c);
		if(b.rb != 0 && a.rb == 0)
			resolve_collision(c);

	}
}

void kinematicBody::update()
{
	velocity += gravity * Time.deltaTime;
	transform.move(velocity * Time.deltaTime);
}
void kinematicBody::onCollision(collision &col)
{
	if (col.other_collider)
	{
		transform.move((col.this_collider->a.getCenter() - col.other_collider->a.getCenter()) / 4.f);
		auto k = col.g_o->getComponent<kinematicBody>();
		if (k != 0)
		{
			std::swap(k->velocity, this->velocity);
		}
		this->velocity *= 0.99f;
		return;
	}

	if (col.normal == glm::vec3(0))
	{
		velocity = glm::reflect(velocity, randomSphere());
	}
	else
	{
		velocity = glm::reflect(velocity, col.normal) * 0.8f;
	}
	// transform->setPosition(glm::vec3(transform.getPosition().x, point.y , transform.getPosition().z));
}

glm::vec3 kinematicBody::gravity = glm::vec3(0, -9.81, 0);

void treenode::clear()
{
	isLeaf = (true);
	// objCounter = 0;
	objs.clear();
}
void treenode::init(bool _left, int _axis, int _parent, int _id)
{
	isLeaf = (true);
	// objCounter = 0;
	axis = _axis;
	id = _id;
	farthest = 0;
	d = 0;
	children = -1;
	left = _left;
	// m.unlock();
}
void treenode::push_back(AABB2 &a, collider *cd)
{
	objs.push_back({a, cd});
	// objCounter++;
	// return objs.back();
}

treenode::treenode() : m() {}
treenode::treenode(const treenode &t) : m() {}

void octree::clear()
{
	nodes.clear();
	nodes.push_back(treenode());
	nodes[0].init(0, 0, 0, 0);
}

void octree::query(collider &myCol, int &colCount, treenode &curr)
{

	if (curr.isLeaf || myCol.a.min[curr.axis] < curr.d + curr.farthest || myCol.a.max[curr.axis] < curr.d - curr.farthest)
	{
		if (curr.left)
			for (int i = 0; i < curr.objs.size(); i++)
			{
				if (myCol.a.min[curr.axis] < curr.objs[i].a.max[curr.axis])
					collider::collide(myCol, *curr.objs[i].d, colCount);
			}
		else
			for (int i = 0; i < curr.objs.size(); i++)
			{
				if (myCol.a.max[curr.axis] > curr.objs[i].a.min[curr.axis])
					collider::collide(myCol, *curr.objs[i].d, colCount);
			}
	}

	if (curr.isLeaf)
		return;

	if (myCol.a.max[curr.axis] > curr.d && myCol.a.min[curr.axis] < curr.d)
	{
		query(myCol, colCount, this->nodes[curr.children]);
		query(myCol, colCount, this->nodes[curr.children + 1]);
	}
	else if (myCol.a.max[curr.axis] < curr.d)
		query(myCol, colCount, this->nodes[curr.children]);
	else if (myCol.a.min[curr.axis] > curr.d)
		query(myCol, colCount, this->nodes[curr.children + 1]);
}
void octree::query(collider &myCol, int &colCount)
{
	query(myCol, colCount, nodes[0]);
}
int octree::split()
{
	lock.lock();
	int size = nodes.size();
	nodes.push_back(treenode());
	nodes.push_back(treenode());
	lock.unlock();
	return size;
}
void octree::insert(AABB2 &a, collider &colliderData, int depth, treenode &curr)
{
	// auto curr = [&]()-> treenode& {return this->nodes[currId];};
	if (!curr.isLeaf)
	{
		if (a.max[curr.axis] < curr.d)
		{
			insert(a, colliderData, depth + 1, nodes[curr.children]);
			// nodes[curr.children].insert(colliderData, depth + 1);
			// return;
		}
		else if (a.min[curr.axis] > curr.d)
		{
			insert(a, colliderData, depth + 1, nodes[curr.children + 1]);
			// nodes[curr.children + 1].insert(colliderData, depth + 1);
			// return;
		}
		else
		{
			curr.m.lock();
			float e = _max(abs(a.min[curr.axis]), abs(a.max[curr.axis])) - curr.d;
			if (e > curr.farthest)
				curr.farthest = e;
			curr.push_back(a, &colliderData);
			curr.m.unlock();
			// return;
		}
	}
	else
	{
		curr.m.lock();
		if (curr.isLeaf)
		{
			if (curr.objs.size() < maxObj)
			{
				curr.push_back(a, &colliderData);
			}
			if (curr.objs.size() >= maxObj)
			{

				curr.children = split();
				nodes[curr.children].init(true, (curr.axis + 1) % 3, curr.id, curr.children);
				nodes[curr.children + 1].init(false, (curr.axis + 1) % 3, curr.id, curr.children + 1);

				for (auto &i : curr.objs)
				{
					curr.d += i.a.getCenter()[curr.axis];
				}
				curr.d /= curr.objs.size();

				curr.farthest = 0;
				int j = 0;
				for (int i = 0; i < curr.objs.size(); i++)
				{

					if (curr.objs[i].a.max[curr.axis] < curr.d)
					{
						// nodes[curr.children].insert(curr.objs[i], 0);
						nodes[curr.children].push_back(curr.objs[i].a, curr.objs[i].d);
					}
					else if (curr.objs[i].a.min[curr.axis] > curr.d)
					{
						// nodes[curr.children + 1].insert(curr.objs[i],0);
						nodes[curr.children + 1].push_back(curr.objs[i].a, curr.objs[i].d);
					}
					else
					{
						float e = _max(abs(curr.objs[i].a.min[curr.axis]), abs(curr.objs[i].a.max[curr.axis])) - curr.d;
						if (e > curr.farthest)
						{
							curr.farthest = e;
						}
						curr.objs[j] = curr.objs[i];
						// setPosInTree(curr.objs[j]->c, curr.objs[j]);
						j++;
					}
				}
				curr.objs.resize(j);
				// curr.objCounter = j;
				curr.isLeaf = (false);
				// m.unlock();
				// return;
			}
		}
		else
		{
			if (a.max[curr.axis] < curr.d)
			{
				// m.unlock();
				insert(a, colliderData, depth + 1, nodes[curr.children]);
				// return;
			}
			else if (a.min[curr.axis] > curr.d)
			{
				// m.unlock();
				insert(a, colliderData, depth + 1, nodes[curr.children + 1]);
				// return;
			}
			else
			{
				float e = _max(abs(a.min[curr.axis]), abs(a.max[curr.axis])) - curr.d;
				if (e > curr.farthest)
					curr.farthest = e;
				curr.push_back(a, &colliderData);
				// m.unlock();
				// return;
			}
		}
		curr.m.unlock();
	}
}
void octree::insert(AABB2 &a, collider &colliderData, int depth)
{
	// treenode& curr = nodes[0];
	insert(a, colliderData, depth, nodes[0]);
}

namespace physics_manager
{
	map<int, octree> collisionLayers;
	map<int, set<int>> collisionGraph;
	tbb::concurrent_unordered_map<Mesh *, MESH> meshes;
}
// octree* Octree = new octree();

int colid = 0;
float size(glm::vec3 a)
{
	return a.x * a.y * a.z;
}

// collider::collider(){
// 	setOBB();
// }
// collider::collider(const collider& rhs){

// }
bool collider::_registerEngineComponent()
{
	return true;
}
void collider::setLayer(int l)
{
	layer = l;
}
void collider::onStart()
{
	// posInTree = 0;
	auto _rb = transform->gameObject()->getComponent<rigidBody>();
	if (_rb != 0)
	{
		rb = _rb;
	}
}
void collider::onDestroy()
{
	// if (posInTree)
	// 	posInTree->valid = false;
	if (this->type == colType::meshType)
	{
		this->m.m->references++;
	}
	this->valid = false;
}
using namespace physics_manager;
glm::vec3 collider::setMesh(Mesh *_m)
{
	// cd.type = 2;
	this->type = meshType;
	if (_m == 0)
	{
		this->m.m = &BOX_MESH;
	}
	else
	{
		if (meshes.find(_m) == meshes.end())
		{
			meshes[_m].points = _m->vertices;
			meshes[_m].tris = _m->indices;
		}
		this->m.m = &meshes.at(_m);
		this->m.m->references++;
		glm::vec3 ret(0);
		for (auto &v : _m->vertices)
		{
			if (ret.x < std::abs(v.x))
				ret.x = std::abs(v.x);
			if (ret.y < std::abs(v.y))
				ret.y = std::abs(v.y);
			if (ret.z < std::abs(v.z))
				ret.z = std::abs(v.z);
		}
		return ret;
	}
	return glm::vec3(1);
}
void collider::setPoint()
{
	// cd.type = 3;
	this->type = pointType;
	this->p = point();
}
void collider::setOBB()
{
	this->type = obbType;
	this->o = OBB();
}
void collider::setSphere()
{
	this->type = sphereType;
	this->s = Sphere();
	this->s.r = 1.f;
}
void collider::setPlane()
{
	this->type = planeType;
	this->pl = Plane();
	this->pl.n = glm::vec3(0, 1, 0);
	this->pl.d = 0;
}
void collider::_update()
{

	// posInTree = 0;
	switch (type)
	{
		case aabbType:
		case obbType:
		case meshType:
		case planeType:
		case sphereType:
		{
			glm::vec3 sc = transform->getScale() * dim;
			// glm::mat3 rot = glm::toMat3(transform->getRotation());
			glm::vec3 r = glm::vec3(length(sc));
			glm::vec3 pos = transform->getPosition();
			a = AABB2(pos - r, pos + r);
			this->collider_shape_updated = false;
		}
		break;
		case pointType:
		{
			if (this->p.pos1 == glm::vec3(0))
			{
				this->p.pos2 = transform->getPosition();
			}
			else
			{
				this->p.pos2 = this->p.pos1;
			}
			this->p.pos1 = transform->getPosition();
			// this->a.max = this->p.pos1;
			// this->a.min = this->p.pos2;
			// if(this->a.max.x < this->a.min.x)
			// 	std::swap(this->a.max.x,this->a.min.x);
			// if(this->a.max.y < this->a.min.y)
			// 	std::swap(this->a.max.y,this->a.min.y);
			// if(this->a.max.z < this->a.min.z)
			// 	std::swap(this->a.max.z,this->a.min.z);

			new (&a.max) glm::vec3(glm::max(this->p.pos1.x, this->p.pos2.x),
								glm::max(this->p.pos1.y, this->p.pos2.y),
								glm::max(this->p.pos1.z, this->p.pos2.z));

			new (&a.min) glm::vec3(glm::min(this->p.pos1.x, this->p.pos2.x),
								glm::min(this->p.pos1.y, this->p.pos2.y),
								glm::min(this->p.pos1.z, this->p.pos2.z));

			// // x
			// if(this->p.pos1.x > this->p.pos2.x){
			// 	this->a.max.x = this->p.pos1.x;this->a.min.x = this->p.pos2.x;
			// }else{
			// 	this->a.max.x = this->p.pos2.x;this->a.min.x = this->p.pos1.x;
			// }
			// // y
			// if(this->p.pos1.y > this->p.pos2.y){
			// 	this->a.max.y = this->p.pos1.y;this->a.min.y = this->p.pos2.y;
			// }else{
			// 	this->a.max.y = this->p.pos2.y;this->a.min.y = this->p.pos1.y;
			// }
			// // z
			// if(this->p.pos1.z > this->p.pos2.z){
			// 	this->a.max.z = this->p.pos1.z;this->a.min.z = this->p.pos2.z;
			// }else{
			// 	this->a.max.z = this->p.pos2.z;this->a.min.z = this->p.pos1.z;
			// }
			this->collider_shape_updated = true;
		}
		break;
		// case planeType:
		// 	a = AABB2(glm::vec3(-1000.f,-1000.f,-1000.f),glm::vec3(1000.f,1000.f,1000.f));
		// 	glm::vec3(-1000.f,-0.1f,-1000.f);
		// 	this->collider_shape_updated = true;
		break;
	}
	// glm::vec3 sc = transform->getScale() * dim;
	// // glm::mat3 rot = glm::toMat3(transform->getRotation());
	// glm::vec3 r = vec3(length(sc));
	// glm::vec3 pos = transform->getPosition();
	// this->a = AABB2(pos - r, pos + r);
	this->type = type;
	// this->c = this;
	this->valid = true;
	this->rb = rb;
	// this->collider_shape_updated = false;

	// int depth = 0;
	// if (layer == 1)
	// 	collisionLayers[layer].insert(cd, depth);
}
void collider::midUpdate()
{
	int depth = 0;
	// if (layer == 1)
	collisionLayers[layer].insert(a, *this, depth);
}

void collider::_lateUpdate()
{
	int colCount = 0;
	for (auto &i : collisionGraph[layer])
	{
		// if(collisionLayers[i].nodes.size() <= collisionLayers[layer].nodes.size())
		collisionLayers[i].query(*this, colCount);
	}

	glm::vec3 center = a.getCenter();
	auto terrains = COMPONENT_LIST(terrain);
	if (terrains->size() > 0)
	{
		terrainHit h = terrain::getHeight(center.x, center.z);
		// if (t != 0)
		// {
		// 	if (a.min.y > t->max_height * t->transform->getScale().y + t->transform->getPosition().y)
		// 		return;
		if (a.min.y < h.height)
		{
			glm::vec3 p = transform->getPosition();
			if (rb != 0)
			{
				rb->vel = glm::reflect(rb->vel, h.normal);
				// rb->vel.y = rb->vel.y >= 0 ? rb->vel.y : -rb->vel.y;
			}
			transform->setPosition(glm::vec3(p.x, h.height + (a.max.y - a.min.y) / 2.f, p.z));
			// transform->gameObject->collide(i.second->transform->gameObject,glm::vec3(p.x, h.height, p.z),h.normal);
			auto g = COMPONENT_LIST(terrain)->get(0)->transform.gameObject();
			collision c{glm::vec3(p.x, h.height, p.z), h.normal, this, 0, g};
			transform->gameObject()->collide(c);
		}
	}
	// }
}

void collider::update_data()
{
	switch (type)
	{
	case obbType: // obb
	{
		glm::vec3 sc = this->transform->getScale() * this->dim;
		glm::mat3 rot = glm::toMat3(this->transform->getRotation());

		o.c = this->transform->getPosition();
		o.u = rot;
		o.e = sc;
	}
	break;
	case sphereType:
	{
		this->s.c = transform->getPosition();
	}
	break;
	case planeType:
	{
		glm::vec3 pos = transform->getPosition();
		this->pl.n = transform->getRotation() * glm::vec3(0,1,0);
		this->pl.d = (pl.n.x * pos.x + pl.n.y * pos.y + pl.n.z * pos.z);
		// pl.n = glm::vec3(0,1,0);
		// pl.d = 0;
	}
	break;
	case meshType:
		break;
	case pointType: // point
	{
		// p.pos2 = p.pos1;
		// p.pos1 = c->transform->getPosition();
	}
	break;
	default:
		break;
	}
}

void assignRigidBody(collider *c, rigidBody *rb)
{
	c->rb = rb;
}

bool testCollision(collider &c1, collider &c2, collision& col)
{

	collider *a;
	collider *b;
	glm::mat4 trans;
	glm::mat4 trans2;
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
	col.g_o = 0;
	col.this_collider = a;
	col.other_collider = b;
	switch (a->type)
	{
	case aabbType: // aabb
		switch (b->type)
		{
		case aabbType: // aabb
			return true;
			break;
		case obbType: // aabb - obb

			break;
		default:
			break;
		}
		break;
	case obbType: // obb
		switch (b->type)
		{
		case obbType: // obb
			return TestOBBOBB(a->o, b->o, col);
			break;
		case meshType: // mesh
			trans = a->transform->getModel();
			trans2 = b->transform->getModel();
			return testOBBMesh(a->o, trans, b->m, trans2, col);
			break;
		case pointType:
			return testPointOBB(b->p, a->o, col);
		default:
			break;
		}
	case meshType: // mesh
		switch (b->type)
		{
		case meshType:
			mesh *m1;
			mesh *m2;
			if (a->m.m->tris.size() <= b->m.m->tris.size())
			{
				m1 = &a->m;
				m2 = &b->m;
				trans = glm::inverse(a->transform->getModel()) * b->transform->getModel();
			}
			else
			{
				m1 = &b->m;
				m2 = &a->m;
				trans = glm::inverse(b->transform->getModel()) * a->transform->getModel();
			}
			return testMeshMesh(*m1, trans, *m2, trans2, col);
			break;
		case pointType: // point
			return testPointMesh(b->p, a->m, a->transform->getPosition(), a->transform->getScale(), a->transform->getRotation(), col);
		default:
			break;
		}
	case pointType: // point
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
	case sphereType:
		switch (b->type)
		{
		case sphereType:
			return TestSphereSphere(a->s, b->s, col);
			break;
		case planeType:
			return TestSpherePlane(a->s, b->pl, col);
			break;
		default:
			break;
		}
		break;
	case planeType:
		switch (b->type)
		{
		default:
			break;
		}
		break;
	default:
		break;
	}
	return false;
}

void collider::ser_edit(ser_mode x, YAML::Node &node_9738469372465)
{
	YAML::Node &node = node_9738469372465;
	SER(layer);
	switch (x)
	{
	case ser_mode::edit_mode:
		static map<colType, string> types = {{aabbType, "aabb"}, {obbType, "obb"}, {meshType, "mesh"}, {pointType, "point"}, {sphereType, "sphere"}, {planeType, "plane"}};
		// static bool isTypeOpen = false;
		if (ImGui::Button(types[this->type].c_str()))
			ImGui::OpenPopup("type");
		// ImGui::SameLine();
		if (ImGui::BeginPopup("type"))
		{
			ImGui::Text("collider type");
			ImGui::Separator();
			for (int i = 0; i < types.size(); i++)
				if (ImGui::Selectable(types[(colType)i].c_str()))
				{
					switch ((colType)i)
					{
					case aabbType:
						/* code */
						break;
					case obbType:
						setOBB();
						break;
					case meshType:
						setMesh(0);
						break;
					case pointType:
						setPoint();
						break;
					case sphereType:
						setSphere();
						break;
					case planeType:
						setPlane();
						break;
					default:
						break;
					}
					this->type = (colType)i;
				}
			ImGui::EndPopup();
		}
		switch (this->type)
		{
		case meshType:
		{

			_model mod;
			mod.id = m.mod;
			renderEdit("model", mod);
			if (m.mod != mod.id)
			{
				m.mod = mod.id;
				this->dim = setMesh(&mod.mesh());
			}
			// ImGui::Text("mesh goes here");
		}
		break;
		case sphereType:
		{
			renderEdit("radius", this->s.r);
		}
		break;
		case planeType:
			// renderEdit("normal", this->pl.n);
			// renderEdit("d", this->pl.d);
			break;
		default:
			break;
		}
		RENDER(dim);
		break;
	case ser_mode::read_mode:
		type = node["type"].as<decltype(type)>();
		layer = node["layer"].as<int>();
		r = node["r"].as<glm::vec3>();
		dim = node["dim"].as<glm::vec3>();

		switch (this->type)
		{
		case aabbType:
			a = node["a"].as<AABB2>();
			break;
		case obbType:
			o = node["o"].as<OBB>();
			break;
		case meshType:
		{
			m = node["m"].as<mesh>();
			_model mod;
			mod.id = m.mod;
			setMesh(&mod.mesh());
		}
		break;
		case pointType:
			p = node["p"].as<point>();
			break;
		case sphereType:
			s = node["s"].as<Sphere>();
			break;
		case planeType:
			pl = node["pl"].as<Plane>();
			break;
		default:
			break;
		}
		break;
	case ser_mode::write_mode:
		// (*_iar) >> boost::serialization::base_object<component>(*this);
		// (*_iar) >> layer >> type >> r >> dim;
		node["type"] = type;
		node["layer"] = layer;
		node["r"] = r;
		node["dim"] = dim;

		switch (this->type)
		{
		case aabbType:
			node["a"] = a;
			break;
		case obbType:
			node["o"] = o;
			break;
		case meshType:
			node["m"] = m;
			break;
		case pointType:
			node["p"] = p;
			break;
		case sphereType:
			node["s"] = s;
			break;
		case planeType:
			node["pl"] = pl;
			break;
		default:
			break;
		}
		break;
	}
}

bool raycast(glm::vec3 p, glm::vec3 dir, glm::vec3 &result)
{

	bool hit = false;
	auto colliders = COMPONENT_LIST(collider);
	cout << "p: " + to_string(p) + " dir: " + to_string(dir) + '\n';
	ray ray(p, dir);
	mutex m;
	result = glm::vec3(numeric_limits<float>::max());
	int size = colliders->size();
	cout << "colliders size: " << size << endl;
	parallelfor(size,
				{
					cout << i << endl;
					float min;
					glm::vec3 q;
					if (colliders->getv(i) && IntersectRayAABB3(ray, colliders->get(i)->a))
					{
						// cout << string(to_string(glm::length(colliders->get(i)->a.min - colliders->get(i)->a.max)) + '\n');
						string name = colliders->get(i)->transform->name();
						if (name == "")
						{
							name = "game object " + to_string(colliders->get(i)->transform.id);
						}
						cout << name + '\n'; // + " inters: " + to_string(IntersectRayAABB(p,dir,colliders->get(i)->a,min,q)) + "tmin: " + to_string(min) + '\n';
						glm::vec3 v = colliders->get(i)->a.getCenter();
						float d = glm::length2(v);
						float closest = glm::length2(result);
						if (closest > d)
						{
							auto _m = lock_guard(m);
							closest = glm::length2(result);
							if (closest > d)
							{
								result = v;
								hit = true;
							}
						}
					}
				});

	glm::vec3 v;
	if (terrain::IntersectRayTerrain(p, dir, v))
	{
		if (glm::length2(v) < glm::length2(result))
		{
			result = v;
			hit = true;
		}
	}
	return hit;
}
