#include "game_engine.h"
#include "initMain.h"
#include <fstream>
#include <iostream>
#include <string>

#include <iomanip>
#include <locale>
#include "BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h"
#include "bullet/btBulletDynamicsCommon.h"

btDynamicsWorld* world;	//every physical object go to the world
btDispatcher* dispatcher;	//what collision algorithm to use?
btCollisionConfiguration* collisionConfig;	//what collision algorithm to use?
btBroadphaseInterface* broadphase;	//should Bullet examine every object, or just what close to each other
btConstraintSolver* solver;					//solve collisions, apply forces, impulses
std::vector<btRigidBody*> bodies;

btRigidBody* addSphere(float rad,float x,float y,float z,float mass)
{
	btTransform t;	//position and rotation
	t.setIdentity();
	t.setOrigin(btVector3(x,y,z));	//put it to x,y,z coordinates
	btBoxShape* sphere=new btBoxShape(btVector3(1,1,1));	//it's a sphere, so use sphereshape
	btVector3 inertia(0,0,0);	//inertia is 0,0,0 for static object, else
	if(mass!=0.0)
		sphere->calculateLocalInertia(mass,inertia);	//it can be determined by this function (for all kind of shapes)
	
	btMotionState* motion=new btDefaultMotionState(t);	//set the position (and motion)
	btRigidBody::btRigidBodyConstructionInfo info(mass,motion,sphere,inertia);	//create the constructioninfo, you can create multiple bodies with the same info
	btRigidBody* body=new btRigidBody(info);	//let's create the body itself
	world->addRigidBody(body);	//and let the world know about it
	bodies.push_back(body);	//to be easier to clean, I store them a vector
	return body;
}

_transform renderSphere(btRigidBody* sphere)
{
	// if(sphere->getCollisionShape()->getShapeType()!=SPHERE_SHAPE_PROXYTYPE)	//only render, if it's a sphere
	// 	return;
	// glColor3f(1,0,0);
	// float r=((btSphereShape*)sphere->getCollisionShape())->getRadius();
	btTransform _t;
	sphere->getMotionState()->getWorldTransform(_t);	//get the transform
	mat4 mat; 
	_t.getOpenGLMatrix(glm::value_ptr(mat));	//OpenGL matrix stores the rotation and orientation
	
	_transform t;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(mat, t.scale, t.rotation, t.position, skew, perspective);
	return t;
}

game_object* physObj;


class physicsObject : public component{

	btRigidBody* rb;
	public:
	void init(float x, float y, float z, vec3 force){
		rb = addSphere(1,x,y,z,1);
		rb->setLinearVelocity(btVector3(force.x,force.y,force.z));
	}

	void update(){
		_transform _t = renderSphere(rb);
		transform->setPosition(_t.position);
		transform->setRotation(_t.rotation);
		transform->setScale(_t.scale);
		// get transform from btTransform
	}
	UPDATE(physicsObject, update);
	COPY(physicsObject);
};


template <class T>
std::string FormatWithCommas(T value)
{
	std::stringstream ss;
	ss.imbue(std::locale(""));
	ss << std::fixed << value;
	return ss.str();
}

using namespace glm;
atomic<int> numCubes(0);
game_object *proto = nullptr;
game_object *ExplosionProto = nullptr;
emitter_prototype_ _expSmoke;
emitter_prototype_ _expFlame;

game_object *ground;


struct bullet {
	emitter_prototype_ primarybullet;
	emitter_prototype_ secondarybullet;
	emitter_prototype_ primaryexplosion;
	emitter_prototype_ secondaryexplosion;
};

map<string, bullet> bullets;


class missle : public component
{
public:
	// rigidBody *rb;
	vec3 vel;
	bullet b;
	glm::vec3 rot;
	vector<particle_emitter*> myEmitters;
	// glm::vec3 dir;
	bool hit = false;
	float life;
	missle() {}
	void onStart()
	{
		rot = randomSphere();
		hit = false;
		life = 0;
		myEmitters = transform->gameObject->getComponents<particle_emitter>();
	}
	void setBullet(const bullet& _b){
		b = _b;
		myEmitters[0]->setPrototype(b.primarybullet);
		myEmitters[1]->setPrototype(b.secondarybullet);
	}
	void update()
	{
		if (!hit)
		{
			transform->move(vel * Time.deltaTime);
			transform->rotate(rot, Time.deltaTime * glm::radians(100.f));
			vel += vec3(0, -9.81, 0) * Time.deltaTime;
		}
		else
		{
			if (life < Time.time)
			{
				transform->gameObject->destroy();
				numCubes.fetch_add(-1);
			}
		}
	}
	void onCollision(game_object *go)
	{
		if (proto == transform->gameObject || hit)
			return;

			myEmitters[0]->setPrototype(b.primaryexplosion);
			myEmitters[1]->setPrototype(b.secondaryexplosion);
			// transform->gameObject->removeComponent<_renderer>();
			hit = true;
			life = Time.time + 0.1f;
	}
	UPDATE(missle, update);
	COPY(missle);
};

class gun : public component
{
	float reload;
public:
	float rof;
	float speed;
	float dispersion;
	bullet ammo;
	void fire()
	{
		reload += rof * Time.deltaTime;
		for (int i = 0; i < (int)reload; i++)
		{
			game_object *go = new game_object(*proto);
			// go->getComponent<rigidBody>()->setVelocity(transform->forward() * 100.f + randomSphere() * randf() * 30.f);
			go->getComponent<missle>()->vel = (transform->forward() + (transform->right() * (randf() - 0.5f) + transform->up() * (randf() - 0.5f)) * dispersion) * speed;
			go->getComponent<missle>()->setBullet(ammo);
			// go->getComponent<cube_sc>()->dir = transform->forward() * 60.f + randomSphere() * randf() * 20.f;
			go->transform->setPosition(transform->getPosition() + transform->forward() * 12.f);
		}
		numCubes.fetch_add((int)reload);
		reload -= (int)reload;
	}
	// UPDATE(gun, update);
	COPY(gun);
};
class player_sc : public component
{
	bool cursorReleased = false;
	float speed = 10.f;
	rigidBody *rb;
	bool flying = true;
	bool jumped = false; // do not fly and jump in same frame
	int framecount = 0;
	vec3 ownSpeed = vec3(0);
	bullet bomb;
	bullet laser;
	vector<gun*> guns;
public:
	terrain *t;

	// void onCollision(game_object *collidee)
	// {
	// 	colliding = true;
	// }

	void onStart()
	{
		rb = transform->gameObject->getComponent<rigidBody>();
		guns = transform->gameObject->getComponents<gun>();
		// bomb = bullets["bomb"];
		guns[0]->ammo = bullets["bomb"];
		guns[0]->rof = 1'000'000 / 60;
		guns[0]->dispersion = 0.5f;
		guns[0]->speed = 100;
		// laser = bullets["laser"];
		guns[1]->ammo = bullets["laser"];
		guns[1]->rof = 1000 / 60;
		guns[1]->dispersion = 0;
		guns[1]->speed = 30;
	}
	void update()
	{

		rb->gravity = true;
		transform->rotate(glm::vec3(0, 1, 0), Input.Mouse.getX() * -0.01f);
		transform->rotate(glm::vec3(1, 0, 0), Input.Mouse.getY() * -0.01f);
		transform->rotate(glm::vec3(0, 0, 1), (Input.getKey(GLFW_KEY_Q) - Input.getKey(GLFW_KEY_E)) * Time.unscaledDeltaTime * -1.f);

		glm::vec3 currVel = rb->getVelocity();

		if (framecount++ > 1)
			cout << "\rcubes: " << FormatWithCommas(numCubes.load()) << " particles: " << FormatWithCommas(atomicCounters->storage->at(particleCounters::liveParticles)) << "  fps: " << 1.f / Time.unscaledSmoothDeltaTime << "                           ";
		if (Input.getKeyDown(GLFW_KEY_R))
		{
			speed *= 2;
		}
		else if (Input.getKeyDown(GLFW_KEY_F))
		{
			speed /= 2;
		}
		if (Input.getKeyDown(GLFW_KEY_P))
		{
			Time.timeScale *= 2;
		}
		else if (Input.getKeyDown(GLFW_KEY_L))
		{
			Time.timeScale /= 2;
		}
		else if (Input.getKeyDown(GLFW_KEY_M))
		{
			Time.timeScale = 1;
		}

		terrainHit h = t->getHeight(transform->getPosition().x, transform->getPosition().z);

				rb->gravity = false;
				vec3 inputVel = ((float)(Input.getKey(GLFW_KEY_A) - Input.getKey(GLFW_KEY_D)) * transform->right() + (float)(Input.getKey(GLFW_KEY_SPACE) - Input.getKey(GLFW_KEY_LEFT_SHIFT)) * transform->up() + (float)(Input.getKey(GLFW_KEY_W) - Input.getKey(GLFW_KEY_S)) * transform->forward());
				if (inputVel.x != 0 || inputVel.z != 0)
					inputVel = normalize(inputVel);
				rb->setVelocity(currVel + inputVel * speed * 5.f);
				glm::vec3 vel = rb->getVelocity();
				rb->setVelocity(glm::vec3(vel.x, vel.y, vel.z) * 0.3f);
		
		if (Input.getKeyDown(GLFW_KEY_SPACE) && jumped)
		{ // pressing jump while airborne begins flight
			flying = true;
		}
		if (Input.getKeyDown(GLFW_KEY_SPACE))
		{ // jump
			glm::vec3 vel = rb->getVelocity();
			jumped = true;
			rb->setVelocity(vec3(vel.x * .5f, .5f * speed, vel.z * .5f));
		}

		if (Input.getKeyDown(GLFW_KEY_ESCAPE) && cursorReleased)
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			cursorReleased = false;
		}
		else if (Input.getKeyDown(GLFW_KEY_ESCAPE) && !cursorReleased)
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			cursorReleased = true;
		}
		if (Input.Mouse.getButton(GLFW_MOUSE_BUTTON_LEFT)){
			for(int i = 0; i < 1; i++){
				auto g = new game_object(*physObj);
				vec3 r = randomSphere() * 2.f * randf() + transform->getPosition() + transform->forward() * 12.f;
				physObj->getComponent<physicsObject>()->init(r.x,r.y,r.z, transform->forward() * 30.f + randomSphere()*10.f);
			}
			// guns[0]->fire();
		}
		if (Input.Mouse.getButton(GLFW_MOUSE_BUTTON_RIGHT)){
			guns[1]->fire();
		}

	}
	UPDATE(player_sc, update);
	COPY(player_sc);
};

class nbody : public component
{
	//    glm::vec3 vel;
	rigidBody *rb;

public:
	void onStart()
	{
		rb = transform->gameObject->getComponent<rigidBody>();
	}
	void update()
	{
		deque<nbody> &v = COMPONENT_LIST(nbody)->data.data;
		glm::vec3 acc(0);
		//        cout << v.size();
		for (auto &i : v)
		{
			glm::vec3 dir = i.transform->getPosition() - transform->getPosition();
			float r = glm::length2(dir);
			if (r < 0.1f || r > 5000.f)
				continue;
			if (!vecIsNan(dir))
				acc += 10.f * glm::normalize(dir) / r;
		}
		if (vecIsNan(acc * Time.deltaTime))
		{
			acc = glm::vec3(0);
		}
		rb->setVelocity(rb->getVelocity() + acc * Time.deltaTime);
		//        transform->move(vel * Time.deltaTime);
	}
	UPDATE(nbody, update);
	COPY(nbody);
};
class spinner : public component
{
	vec3 axis;
	public:
	void update(){
		transform->rotate(axis,0.1f * Time.deltaTime);
	}
	void onStart(){
		axis = normalize(randomSphere());
	}
	UPDATE(spinner,update);
	COPY(spinner);
};


class autoCubes : public component
{

public:
	gun* g;
	void onStart(){
		g = transform->gameObject->getComponent<gun>();
	}
	void update()
	{
		g->fire();
	}
	UPDATE(autoCubes, update);
	COPY(autoCubes);
};


typedef char byte_t;


static int s_gridSize = 128 + 1;  // must be (2^N) + 1
static btScalar s_gridSpacing = 0.5;
static btScalar s_gridHeightScale = 0.02;

static btScalar
convertToFloat
(
	const char * p,
	PHY_ScalarType type
)
{
	btAssert(p);

	switch (type) {
	case PHY_FLOAT:
	{
		btScalar * pf = (btScalar *)p;
		return *pf;
	}

	case PHY_UCHAR:
	{
		unsigned char * pu = (unsigned char *)p;
		return ((*pu) * s_gridHeightScale);
	}

	case PHY_SHORT:
	{
		short * ps = (short *)p;
		return ((*ps) * s_gridHeightScale);
	}

	default:
		btAssert(!"bad type");
	}

	return 0;
}
static void
convertFromFloat
(
	byte_t * p,
	btScalar value,
	PHY_ScalarType type
)
{
	btAssert(p && "null");

	switch (type) {
	case PHY_FLOAT:
	{
		btScalar * pf = (btScalar *)p;
		*pf = value;
	}
	break;

	case PHY_UCHAR:
	{
		unsigned char * pu = (unsigned char *)p;
		*pu = (unsigned char)(value / s_gridHeightScale);
	}
	break;

	case PHY_SHORT:
	{
		short * ps = (short *)p;
		*ps = (short)(value / s_gridHeightScale);
	}
	break;

	default:
		btAssert(!"bad type");
	}
}


// creates a radially-varying heightfield
static void
setRadial
(
	char * grid,
	int bytesPerElement,
	PHY_ScalarType type,
	btScalar phase = 0.0
)
{
	btAssert(grid);
	btAssert(bytesPerElement > 0);

	// min/max
	btScalar period = 0.5 / s_gridSpacing;
	btScalar floor = 0.0;
	btScalar min_r = 3.0 * btSqrt(s_gridSpacing);
	btScalar magnitude = 5.0 * btSqrt(s_gridSpacing);

	// pick a base_phase such that phase = 0 results in max height
	//   (this way, if you create a heightfield with phase = 0,
	//    you can rely on the min/max heights that result)
	btScalar base_phase = (0.5 * SIMD_PI) - (period * min_r);
	phase += base_phase;

	// center of grid
	btScalar cx = 0.5 * s_gridSize * s_gridSpacing;
	btScalar cy = cx;		// assume square grid
	char * p = grid;
	for (int i = 0; i < s_gridSize; ++i) {
		float x = i * s_gridSpacing;
		for (int j = 0; j < s_gridSize; ++j) {
			float y = j * s_gridSpacing;

			float dx = x - cx;
			float dy = y - cy;

			float r = sqrt((dx * dx) + (dy * dy));

			float z = period;
			if (r < min_r) {
				r = min_r;
			}
			z = (1.0 / r) * sin(period * r + phase);
			if (z > period) {
				z = period;
			}
			else if (z < -period) {
				z = -period;
			}
			z = floor + magnitude * z;

			convertFromFloat(p, z, type);
			p += bytesPerElement;
		}
	}
}








int main(int argc, char **argv)
{
	if (argc > 1)
		sort1 = stoi(argv[1]);
	if (argc > 2)
		maxGameDuration = (float)stoi(argv[2]);

	hideMouse = false;
	
	//pretty much initialize everything logically
	collisionConfig=new btDefaultCollisionConfiguration();
	dispatcher=new btCollisionDispatcher(collisionConfig);
	broadphase=new btDbvtBroadphase();
	solver=new btSequentialImpulseConstraintSolver();
	world=new btDiscreteDynamicsWorld(dispatcher,broadphase,solver,collisionConfig);
	world->setGravity(btVector3(0,-10,0));	//gravity on Earth
	
	init();
	initmain()
	//////////////////////////////////////////////////////////

	physObj = new game_object();
	physObj->addComponent<_renderer>()->set(modelShader, cubeModel);
	physObj->addComponent<physicsObject>()->init(0,20,0,vec3(0));

	for(int i = 0; i < 60; i++){
		auto g = new game_object(*physObj);
		vec3 r = randomSphere() * 500.f * randf() + vec3(0,500,0);
		physObj->getComponent<physicsObject>()->init(r.x,r.y,r.z,vec3(0));
	}


	player = new game_object();
	auto playerCam = player->addComponent<_camera>();
	playerCam->fov = 60;
	playerCam->farPlane = 1e32f;
	player->addComponent<gun>();
	player->addComponent<gun>();
	// player->addComponent<collider>()->layer = 1;
	player->addComponent<rigidBody>()->bounciness = 0.3;
	// player->addComponent<rigidBody>()->gravity = false;
	player->addComponent<player_sc>();
	player->transform->translate(vec3(0, 10, -10));
	ground = new game_object();

	// ground->transform->scale(vec3(20));
	auto r = ground->addComponent<_renderer>();
	r->set(modelShader, terrainModel);
	auto t = ground->addComponent<terrain>();
	t->r = r;
	// t->width = t->depth = 1024;
	t->genHeightMap(128,128);
	 ground->transform->translate(glm::vec3(-64,0,-64));
	// ground->transform->translate(glm::vec3(-10240, -5000, -10240));

	float minH = 10000;
	float maxH = -10000;
	float *heightData = new float[128*128];
	// setRadial(heightData, 4, PHY_FLOAT);
	int x = 0;
	for(int i = 0; i < t->heightMap.size(); i++){
		for(int j = 0; j < t->heightMap[i].size(); j++){
			heightData[j * 128 + i] = t->heightMap[i][j];
			if(minH > heightData[i*128 + j])
				minH = heightData[i*128 + j];
			else if(maxH < heightData[i*128 + j])
				maxH = heightData[i*128 + j];
		}
	}
	// for(auto &i : t->heightMap){
	// 	for(auto &j : i){
	// 		// convertFromFloat(&heightData[x*4],j,PHY_FLOAT);
	// 		heightData[x] = j;
	// 		if(minH > j)
	// 			minH = j;
	// 		else if(maxH < j)
	// 			maxH = j;
	// 		x++;
	// 	}
	// }


	//similar to createSphere
	btTransform ter;
	ter.setIdentity();
	ter.setOrigin(btVector3(0,maxH/2,0));
	btStaticPlaneShape* plane=new btStaticPlaneShape(btVector3(0,1,0),0);
	btHeightfieldTerrainShape* heightField =
		new btHeightfieldTerrainShape(128,128,
			heightData,btScalar(1.0),btScalar(0.0),btScalar(maxH),1,PHY_FLOAT,false);
	btVector3 localScaling(1,1,1);
	heightField->setLocalScaling(localScaling);
	btMotionState* motion=new btDefaultMotionState(ter);
	// btVector3 localInertia(0,0,0);
	btRigidBody::btRigidBodyConstructionInfo info(0.0,motion,heightField);
	btRigidBody* body=new btRigidBody(info);
	world->addRigidBody(body);
	bodies.push_back(body);


	player->getComponent<player_sc>()->t = t;
	ifstream config("config.txt");
	int n;
	config >> n;
	numCubes = n;
	srand(100);

	
	game_object *CUBE = new game_object();
	// CUBE->addComponent<_renderer>();
	// CUBE->addComponent<rigidBody>()->setVelocity(vec3(0));
	// CUBE->getComponent<rigidBody>()->bounciness = .98f; //->gravity = false; //
	CUBE->addComponent<collider>()->layer = 0;
	// CUBE->getComponent<_renderer>()->set(modelShader, cubeModel);
	auto pe2 = CUBE->addComponent<particle_emitter>();
	// pe2->setPrototype(flameEmitterProto);
	auto pe3 = CUBE->addComponent<particle_emitter>();
	// pe3->setPrototype(smokeEmitter);
	CUBE->addComponent<missle>()->setBullet(bomb);

	

	////////////////////////////////////////////////

	//gameObjects.front()->addComponent<mvpSolver>();

	proto = CUBE;

	game_object *shooter = new game_object();
	shooter->transform->move(vec3(0, 100, 0));
	shooter->addComponent<_renderer>()->set(modelShader, cubeModel);
	gun* g = shooter->addComponent<gun>();
	g->rof = 8;
	g->dispersion = 0.3;
	g->speed = 100;
	g->ammo = bullets["bomb"];
	shooter->addComponent<autoCubes>();
	shooter->addComponent<collider>()->layer = 1;
	shooter->transform->setScale(vec3(6));
	game_object *go = new game_object(*shooter);

	auto nanosuitMan = new game_object(*CUBE);
	nanosuitMan->addComponent<_renderer>();
	nanosuitMan->getComponent<_renderer>()->set(modelShader, nanoSuitModel);
	// cube_sc *it = nanosuitMan->getComponent<cube_sc>();
	// nanosuitMan->removeComponent<cube_sc>(it);
	nanosuitMan->transform->move(glm::vec3(-10.f));

	emitter_prototype_ ep2 = createNamedEmitter("emitter2");
	*ep2 = *flameEmitterProto;
	ep2->billboard = 0;
	ep2->trail = 0;
	ep2->emission_rate = 5.0f;
	ep2->lifetime = 7.f;
	ep2->velocity = vec3(1);
	ep2->color = vec4(1, .4, 0, 0.5);
	auto pe = nanosuitMan->addComponent<particle_emitter>();
	pe->setPrototype(ep2);

	game_object *proto2 = new game_object();
	proto2->transform->translate(vec3(50));
	proto2->addComponent<spinner>();
	proto2->addComponent<_renderer>();
	proto2->addComponent<rigidBody>()->gravity = false;
	proto2->addComponent<collider>()->layer = 1;
	// proto2->addComponent<cube_sc>();
	proto2->getComponent<_renderer>()->set(modelShader, cubeModel);
	proto2->addComponent<particle_emitter>();
	// proto2->removeComponent<cube_sc>();
	// proto2->removeComponent<particle_emitter>();
	proto2->getComponent<particle_emitter>()->setPrototype(ep2);
	proto2->transform->setScale(glm::vec3(10.f));
	proto2->transform->translate(glm::vec3(10.f) * 0.5f);

	// create big cubes
	for (int i = 0; i < 0; ++i)//20
	{
		proto2 = new game_object(*proto2);
		proto2->transform->setScale(glm::vec3(pow(10.f, (float)(i + 1)),pow(10.f, (float)(i + 1)),pow(11.f, (float)(i + 1))));
		proto2->transform->translate(glm::vec3(pow(10.f, (float)(i + 1))) * 2.f);
		proto2->transform->rotate(randomSphere(),randf() * 1.5);
	}

	// create shooters
	for (int i = 0; i < 0; ++i)
	{
		go = new game_object(*go);
		go->transform->translate(randomSphere() * 1000.f);
		vec3 pos = go->transform->getPosition();
		go->transform->setPosition(vec3(fmod(pos.x, 8000), fmod(pos.y, 300.f) + 100.f, fmod(pos.z, 8000)));
		go->transform->rotate(randomSphere(), randf() * 10.f);
		// if (fmod((float)i, (n / 100)) < 0.01)
		// cout << "\r" << (float)i / (float)n << "    " << flush;
	}

	// create blob of bombs
	go = new game_object(*CUBE);
	for (int i = 0; i < n; i++)
	{
		go = new game_object(*go);
		go->transform->translate(randomSphere() * 3.f);
		if (fmod((float)i, (n / 100)) < 0.01)
			cout << "\r" << (float)i / (float)n << "    " << flush;
		go->getComponent<missle>()->vel = randomSphere() * randf() * 100.f;
	}



	run(world);
	for(int i=0;i<bodies.size();i++)
	{
		world->removeCollisionObject(bodies[i]);
		btMotionState* motionState=bodies[i]->getMotionState();
		btCollisionShape* shape=bodies[i]->getCollisionShape();
		delete bodies[i];
		delete shape;
		delete motionState;
	}
	delete dispatcher;
	delete collisionConfig;
	delete solver;
	delete broadphase;
	delete world;
	delete[] heightData;


	return 0;
}
