#include "game_engine.h"
#include <fstream>
#include <iostream>
#include <string>

#include <iomanip>
#include <locale>

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
			go->transform->setPosition(transform->getPosition() + transform->forward()*10.f - transform->up()*20.f);
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
		guns[1]->speed = 60000;
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


		// if (transform->getPosition().z < h.height + 1.f)
		// { // if grounded
		// 	// rb->gravity = true;
		// 	flying = false;
		// 	jumped = false;
		// 	transform->setPosition(vec3(transform->getPosition().x, h.height + 1.1f, transform->getPosition().z));
		// 	vec3 inputVel = ((float)(Input.getKey(GLFW_KEY_A) - Input.getKey(GLFW_KEY_D)) * transform->right() + (float)(Input.getKey(GLFW_KEY_W) - Input.getKey(GLFW_KEY_S)) * transform->forward());

		// 	glm::vec3 vel = rb->getVelocity();
		// 	// if (transform->getPosition().y - 1.0f <= h.height)
		// 	// 	rb->setVelocity(vec3(vel.x, 0, vel.y) * 0.5f);
		// 	if (inputVel.x != 0 || inputVel.z != 0)
		// 	{

		// 		inputVel = normalize(inputVel);
		// 		vec3 temp = cross(inputVel, h.normal);
		// 		inputVel = normalize(cross(h.normal, temp));
		// 		rb->setVelocity(currVel + inputVel * speed);
		// 	}
		// 	if (Input.getKeyDown(GLFW_KEY_SPACE))
		// 	{ // jump
		// 		jumped = true;
		// 		rb->setVelocity(vec3(vel.x * .5f, speed + vel.y, vel.z * .5f));
		// 	}
		// 	if (h.height == -INFINITY)
		// 	{
		// 		jumped = true;
		// 		return;
		// 	}
		// 	glm::vec3 currPos = transform->getPosition();
		// 	// currPos.y = h + .99f;
		// 	// transform->setPosition(currPos);
		// }
		// if (transform->getPosition().y - 1.0f > h.height)
		// {
		// 	if (flying)
		// 	{ // if flying dont apply gravity
				rb->gravity = false;
				vec3 inputVel = ((float)(Input.getKey(GLFW_KEY_A) - Input.getKey(GLFW_KEY_D)) * transform->right() + (float)(Input.getKey(GLFW_KEY_SPACE) - Input.getKey(GLFW_KEY_LEFT_SHIFT)) * transform->up() + (float)(Input.getKey(GLFW_KEY_W) - Input.getKey(GLFW_KEY_S)) * transform->forward());
				if (inputVel.x != 0 || inputVel.z != 0)
					inputVel = normalize(inputVel);
				rb->setVelocity(currVel + inputVel * speed * 5.f);
				glm::vec3 vel = rb->getVelocity();
				rb->setVelocity(glm::vec3(vel.x, vel.y, vel.z) * 0.3f);
			// }
			// else
			// {
			// 	rb->gravity = true;
			// 	vec3 inputVel = ((float)(Input.getKey(GLFW_KEY_A) - Input.getKey(GLFW_KEY_D)) * transform->right() + (float)(Input.getKey(GLFW_KEY_SPACE) - Input.getKey(GLFW_KEY_LEFT_SHIFT)) * transform->up() + (float)(Input.getKey(GLFW_KEY_W) - Input.getKey(GLFW_KEY_S)) * transform->forward());
			// 	if (inputVel.x != 0 || inputVel.z != 0)
			// 		inputVel = normalize(inputVel);
			// 	ownSpeed += inputVel * speed * 0.1f;
			// 	if (length2(ownSpeed) < speed * speed)
			// 		rb->setVelocity(currVel + inputVel * speed * 0.1f);
			// 	glm::vec3 vel = rb->getVelocity();
			// 	// rb->setVelocity(vel - vec3(speed,0,speed) * 0.3f);
			// }
		// }
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
			guns[0]->fire();
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
int main(int argc, char **argv)
{
	if (argc > 1)
		sort1 = stoi(argv[1]);
	if (argc > 2)
		maxGameDuration = (float)stoi(argv[2]);

	hideMouse = false;
	init();
	_shader particleShader("res/shaders/particles.vert", "res/shaders/particles.geom", "res/shaders/particles.frag");
	_shader modelShader("res/shaders/model.vert", "res/shaders/model.frag");
	_model cubeModel("res/models/cube/cube.obj");
	_model nanoSuitModel("res/models/nanosuit/nanosuit.obj");
	_model terrainModel("res/models/terrain/terrain.obj");
	//	waitForRenderQueue();

	collisionGraph[0] = {1};
	collisionGraph[1] = {0,1};

	///////////////////////////////////////////////////

	emitter_prototype_ flameEmitterProto = createNamedEmitter("flame");
	flameEmitterProto->emission_rate = 3.f;
	flameEmitterProto->lifetime = 0.67f;
	flameEmitterProto->color = vec4(1, 1, 0.1f, 0.8f);
	flameEmitterProto->velocity = vec3(1.f);
	flameEmitterProto->scale = vec3(1.5f);
	flameEmitterProto->billboard = 1;
	flameEmitterProto->trail = 1;

	emitter_prototype_ smokeEmitter = createNamedEmitter("smoke");
	*smokeEmitter = *flameEmitterProto;
	smokeEmitter->emission_rate = 2.f;
	smokeEmitter->lifetime = 3.f;
	smokeEmitter->color = vec4(0.5f, 0.5f, 0.5f, 0.2f);
	smokeEmitter->velocity = vec3(4.f);
	smokeEmitter->scale = vec3(3);
	smokeEmitter->trail = 1;

	////////////////////////////////////////////////

	emitter_prototype_ emitterProto2 = createNamedEmitter("expflame");
	emitterProto2->emission_rate = 50.f;
	emitterProto2->lifetime = 1.f;
	emitterProto2->color = vec4(1, 1, 0.2f, 0.8f);
	emitterProto2->velocity = vec3(60.f);
	emitterProto2->scale = vec3(25.f);
	emitterProto2->billboard = 1;
	emitterProto2->trail = 0;
	_expFlame = emitterProto2;

	emitter_prototype_ emitterProto4 = createNamedEmitter("expsmoke");
	*emitterProto4 = *emitterProto2;
	emitterProto4->emission_rate = 50.f;
	emitterProto4->lifetime = 6.f;
	// emitterProto3->trail = 0;
	emitterProto4->scale = vec3(20);
	emitterProto4->velocity = vec3(30.f);
	emitterProto4->color = vec4(0.45f);
	_expSmoke = emitterProto4;
	/////////////////////////////////////////////
	bullet bomb;
	bomb.primarybullet = flameEmitterProto;
	bomb.secondarybullet = smokeEmitter;
	bomb.primaryexplosion = emitterProto2;
	bomb.secondaryexplosion = emitterProto4;
	bullets["bomb"] = bomb;

	bullet laser;
	laser.primarybullet = createNamedEmitter("laserbeam");
	laser.primarybullet->color = vec4(.8,.8,1,1);
	laser.primarybullet->lifetime = 0.3f;
	laser.primarybullet->emission_rate = 20.f;
	laser.primarybullet->trail = 1;
	laser.primarybullet->scale = vec3(20.f);
	laser.primarybullet->velocity = vec3(1.f);
	laser.secondarybullet = createNamedEmitter("laserbeam2");
	laser.secondarybullet->color = vec4(.2,.2,1,0.5);
	laser.secondarybullet->lifetime = 1.f;
	laser.secondarybullet->emission_rate = 20.f;
	laser.secondarybullet->trail = 1;
	laser.secondarybullet->scale = vec3(25.f);
	laser.secondarybullet->velocity = vec3(1.f);
	laser.primaryexplosion = createNamedEmitter("beamexplosion1");
	laser.primaryexplosion->color = vec4(.8,.8,1,1);
	laser.primaryexplosion->scale = vec3(1000);
	laser.primaryexplosion->lifetime = 1.f;
	laser.primaryexplosion->emission_rate = 50.f;
	laser.primaryexplosion->trail = 0;
	laser.primaryexplosion->velocity = vec3(2000.f);
	laser.secondaryexplosion = createNamedEmitter("beamexplosion2");
	laser.secondaryexplosion->color = vec4(.2,.2,1,0.5);
	laser.secondaryexplosion->scale = vec3(1000);
	laser.secondaryexplosion->lifetime = 6.f;
	laser.secondaryexplosion->emission_rate = 50.f;
	laser.secondaryexplosion->trail = 0;
	laser.secondaryexplosion->velocity = vec3(600.f);
	bullets["laser"] = laser;

	bullet blackLaser;
	blackLaser.primarybullet = createNamedEmitter("blacklaserbeam");
	blackLaser.primarybullet->color = vec4(0,0,0,1);
	blackLaser.primarybullet->lifetime = 0.7f;
	blackLaser.primarybullet->emission_rate = 20.f;
	blackLaser.primarybullet->trail = 1;
	blackLaser.primarybullet->scale = vec3(100.f);
	blackLaser.primarybullet->velocity = vec3(1.f);
	blackLaser.secondarybullet = createNamedEmitter("blacklaserbeam2");
	blackLaser.secondarybullet->color = vec4(1,0,0,0.5);
	blackLaser.secondarybullet->lifetime = 1.f;
	blackLaser.secondarybullet->emission_rate = 20.f;
	blackLaser.secondarybullet->trail = 1;
	blackLaser.secondarybullet->scale = vec3(160.f);
	blackLaser.secondarybullet->velocity = vec3(1.f);
	blackLaser.primaryexplosion = createNamedEmitter("blackbeamexplosion1");
	blackLaser.primaryexplosion->color = vec4(0,0,0,1);
	blackLaser.primaryexplosion->scale = vec3(10000);
	blackLaser.primaryexplosion->lifetime = 3.f;
	blackLaser.primaryexplosion->emission_rate = 100.f;
	blackLaser.primaryexplosion->trail = 0;
	blackLaser.primaryexplosion->velocity = vec3(10000.f);
	blackLaser.secondaryexplosion = createNamedEmitter("blackbeamexplosion2");
	blackLaser.secondaryexplosion->color = vec4(0.8,0,0,0.5);
	blackLaser.secondaryexplosion->scale = vec3(15000);
	blackLaser.secondaryexplosion->lifetime = 6.f;
	blackLaser.secondaryexplosion->emission_rate = 40.f;
	blackLaser.secondaryexplosion->trail = 0;
	blackLaser.secondaryexplosion->velocity = vec3(5000.f);
	bullets["black"] = blackLaser;

	player = new game_object();
	auto playerCam = player->addComponent<_camera>();
	playerCam->fov = 60;
	playerCam->farPlane = 1e32f;
	player->addComponent<gun>();
	player->addComponent<gun>();
	player->addComponent<collider>()->layer = 1;
	player->addComponent<rigidBody>()->bounciness = 0.3;
	// player->addComponent<rigidBody>()->gravity = false;
	player->addComponent<player_sc>();
	player->transform->translate(vec3(0, 0, -10120));
	ground = new game_object();

	ground->transform->scale(vec3(20));
	auto r = ground->addComponent<_renderer>();
	r->set(modelShader, terrainModel);
	auto t = ground->addComponent<terrain>();
	t->r = r;
	t->width = t->depth = 1024;
	ground->transform->translate(glm::vec3(-10240, -5000, -10240));

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
	for (int i = 0; i < 20; ++i)
	{
		proto2 = new game_object(*proto2);
		proto2->transform->setScale(glm::vec3(pow(10.f, (float)(i + 1))));
		proto2->transform->translate(glm::vec3(pow(10.f, (float)(i + 1))) * 2.f);
		proto2->transform->rotate(randomSphere(),randf() * 1.5);
	}

	for (int i = 0; i < 1000; ++i)
	{
		go = new game_object(*go);
		go->transform->translate(randomSphere() * 1000.f);
		vec3 pos = go->transform->getPosition();
		go->transform->setPosition(vec3(fmod(pos.x, 8000), fmod(pos.y, 300.f) + 100.f, fmod(pos.z, 8000)));
		go->transform->rotate(randomSphere(), randf() * 10.f);
		// if (fmod((float)i, (n / 100)) < 0.01)
		// cout << "\r" << (float)i / (float)n << "    " << flush;
	}

	go = new game_object(*CUBE);
	for (int i = 0; i < n; i++)
	{
		go = new game_object(*go);
		go->transform->translate(randomSphere() * 3.f);
		if (fmod((float)i, (n / 100)) < 0.01)
			cout << "\r" << (float)i / (float)n << "    " << flush;
		go->getComponent<missle>()->vel = randomSphere() * randf() * 100.f;
	}



	run();

	return 0;
}
