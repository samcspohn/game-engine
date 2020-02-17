#include "game_engine.h"
#include <fstream>
#include <iostream>
#include <string>

#include <iomanip>
#include <locale>

template<class T>
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

game_object *ground;
class player_sc : public component
{
	bool cursorReleased = false;
	float speed = 10.f;
	rigidBody *rb;
	bool flying = false;
	bool jumped = false; // do not fly and jump in same frame
	bool colliding = false;
	int framecount = 0;
	vec3 ownSpeed = vec3(0);

public:
	terrain *t;

	void onCollision(game_object *collidee)
	{
		colliding = true;
	}

	void onStart()
	{
		rb = transform->gameObject->getComponent<rigidBody>();
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
		if (colliding)
		{ // if grounded
			// rb->gravity = true;
			flying = false;
			jumped = false;
			transform->setPosition(vec3(transform->getPosition().x, h.height + 1.f, transform->getPosition().z));
			vec3 inputVel = ((float)(Input.getKey(GLFW_KEY_A) - Input.getKey(GLFW_KEY_D)) * transform->right() + (float)(Input.getKey(GLFW_KEY_W) - Input.getKey(GLFW_KEY_S)) * transform->forward());

			glm::vec3 vel = rb->getVelocity();
			// if (transform->getPosition().y - 1.0f <= h.height)
			// 	rb->setVelocity(vec3(vel.x, 0, vel.y) * 0.5f);
			if (inputVel.x != 0 || inputVel.z != 0)
			{

				inputVel = normalize(inputVel);
				vec3 temp = cross(inputVel, h.normal);
				inputVel = normalize(cross(h.normal, temp));
				rb->setVelocity(currVel + inputVel * speed);
			}
			if (Input.getKeyDown(GLFW_KEY_SPACE))
			{ // jump
				jumped = true;
				rb->setVelocity(vec3(vel.x * .5f, speed + vel.y, vel.z * .5f));
			}
			if (h.height == -INFINITY)
			{
				jumped = true;
				return;
			}
			glm::vec3 currPos = transform->getPosition();
			// currPos.y = h + .99f;
			// transform->setPosition(currPos);
		}
		if (transform->getPosition().y - 1.0f > h.height)
		{

			if (flying)
			{ // if flying dont apply gravity
				rb->gravity = true;
				vec3 inputVel = ((float)(Input.getKey(GLFW_KEY_A) - Input.getKey(GLFW_KEY_D)) * transform->right() + (float)(Input.getKey(GLFW_KEY_SPACE) - Input.getKey(GLFW_KEY_LEFT_SHIFT)) * transform->up() + (float)(Input.getKey(GLFW_KEY_W) - Input.getKey(GLFW_KEY_S)) * transform->forward());
				if (inputVel.x != 0 || inputVel.z != 0)
					inputVel = normalize(inputVel);
				rb->setVelocity(currVel + inputVel * speed * 5.f);
				glm::vec3 vel = rb->getVelocity();
				rb->setVelocity(glm::vec3(vel.x, vel.y, vel.z) * 0.3f);
			}
			else
			{
				rb->gravity = true;
				vec3 inputVel = ((float)(Input.getKey(GLFW_KEY_A) - Input.getKey(GLFW_KEY_D)) * transform->right() + (float)(Input.getKey(GLFW_KEY_SPACE) - Input.getKey(GLFW_KEY_LEFT_SHIFT)) * transform->up() + (float)(Input.getKey(GLFW_KEY_W) - Input.getKey(GLFW_KEY_S)) * transform->forward());
				if (inputVel.x != 0 || inputVel.z != 0)
					inputVel = normalize(inputVel);
				ownSpeed += inputVel * speed * 0.1f;
				if (length2(ownSpeed) < speed * speed)
					rb->setVelocity(currVel + inputVel * speed * 0.1f);
				glm::vec3 vel = rb->getVelocity();
				// rb->setVelocity(vel - vec3(speed,0,speed) * 0.3f);
			}
		}
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
		colliding = false;

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
	}
	UPDATE(player_sc, update);
	COPY(player_sc);
};

class explosion_sc : public component
{
	float life;

public:
	void onStart()
	{
		life = 0;
	}
	void update()
	{
		if (transform->gameObject == ExplosionProto)
			return;
		life += Time.deltaTime;
		if (life > 0.1f)
		{
			transform->gameObject->destroy();
		}
	}
	UPDATE(explosion_sc, update);
	COPY(explosion_sc);
};

class cube_sc : public component
{
public:
	rigidBody *rb;
	glm::vec3 rot;
	// glm::vec3 dir;
	bool hit = false;
	cube_sc() {}
	void onStart()
	{
		rot = randomSphere();
		// dir = randomSphere() * 15.f;
		rb = transform->gameObject->getComponent<rigidBody>();
		rb->setVelocity(randomSphere() * randf() * 3.f);
	}
	void update()
	{
		// transform->translate(dir * Time.deltaTime * 10.f);
		transform->rotate(rot, Time.deltaTime * glm::radians(100.f));
		if (Input.getKey(GLFW_KEY_C) && proto != transform->gameObject && randf() < 600.f / numCubes * Time.deltaTime || hit)
		{
			numCubes.fetch_add(-1);
			transform->gameObject->destroy();
		}
	}
	void onCollision(game_object *go)
	{
		if( proto == transform->gameObject )
			return;

		if (go->getComponent<terrain>() != 0)
		{
			auto exp = new game_object(*ExplosionProto);
			exp->transform->setPosition(transform->getPosition());
			numCubes.fetch_add(-1);
			transform->gameObject->destroy();
			// cout << "hit" << flush;
			// hit = true;
		}
	}
	UPDATE(cube_sc, update);
	COPY(cube_sc);
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

class moreCUBES : public component
{

public:
	float rof = 1000000 / 60;
	void update()
	{
		if (Input.Mouse.getButton(GLFW_MOUSE_BUTTON_1))
		{
			float r = rof * Time.deltaTime;
			for (int i = 0; i < r; i++)
			{
				game_object *go = new game_object(*proto);
				go->getComponent<rigidBody>()->setVelocity(transform->forward() * 100.f + randomSphere() * randf() * 30.f);
				// go->getComponent<cube_sc>()->dir = transform->forward() * 60.f + randomSphere() * randf() * 20.f;
				go->transform->setPosition(transform->getPosition() + transform->forward() * 30.f - transform->up() * 20.f);
				numCubes.fetch_add(1);
			}
		}
	}
	UPDATE(moreCUBES, update);
	COPY(moreCUBES);
};
int main(void)
{
	// hideMouse = false;
	init();
	_shader particleShader("res/shaders/particles.vert", "res/shaders/particles.geom", "res/shaders/particles.frag");
	_shader modelShader("res/shaders/model.vert", "res/shaders/model.frag");
	_model cubeModel("res/models/cube/cube.obj");
	_model nanoSuitModel("res/models/nanosuit/nanosuit.obj");
	_model terrainModel("res/models/terrain/terrain.obj");
	//	waitForRenderQueue();

	player = new game_object();
	player->addComponent<moreCUBES>();
	// player->addComponent<collider>();
	player->addComponent<rigidBody>()->bounciness = 0.03f;
	player->addComponent<player_sc>();
	player->transform->translate(vec3(0,0,-5120));
	ground = new game_object();

	ground->transform->scale(vec3(10));
	auto r = ground->addComponent<_renderer>();
	r->set(modelShader, terrainModel);
	auto t = ground->addComponent<terrain>();
	t->r = r;
	t->width = t->depth = 1024;
	ground->transform->translate(glm::vec3(-5120, -2050, -5120));

	player->getComponent<player_sc>()->t = t;
	ifstream config("config.txt");
	int n;
	config >> n;
	numCubes = n;
	srand(100);

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
	smokeEmitter->lifetime = 4.f;
	smokeEmitter->color = vec4(0.5f);
	smokeEmitter->velocity = vec3(4.f);
	smokeEmitter->scale = vec3(3);
	smokeEmitter->trail = 1;

	game_object *CUBE = new game_object();
	CUBE->addComponent<_renderer>();
	CUBE->addComponent<rigidBody>()->setVelocity(vec3(0));
	CUBE->getComponent<rigidBody>()->bounciness = .98f; //->gravity = false; //
	CUBE->addComponent<collider>();
	CUBE->addComponent<cube_sc>();
	CUBE->getComponent<_renderer>()->set(modelShader, cubeModel);
	auto pe2 = CUBE->addComponent<particle_emitter>();
	pe2->setPrototype(flameEmitterProto);
	auto pe3 = CUBE->addComponent<particle_emitter>();
	pe3->setPrototype(smokeEmitter);

	////////////////////////////////////////////////
	emitter_prototype_ emitterProto2 = createNamedEmitter("expflame");
	emitterProto2->emission_rate = 100.f;
	emitterProto2->lifetime = 1.f;
	emitterProto2->color = vec4(1, 1, 0.2f, 0.8f);
	emitterProto2->velocity = vec3(60.f);
	emitterProto2->scale = vec3(25.f);
	emitterProto2->billboard = 1;
	emitterProto2->trail = 0;

	emitter_prototype_ emitterProto4 = createNamedEmitter("expsmoke");
	*emitterProto4 = *emitterProto2;
	emitterProto4->emission_rate = 300.f;
	emitterProto4->lifetime = 6.f;
	// emitterProto3->trail = 0;
	emitterProto4->scale = vec3(20);
	emitterProto4->velocity = vec3(30.f);
	emitterProto4->color = vec4(0.45f);

	game_object *explosionProto = new game_object();
	// explosionProto->addComponent<collider>();
	explosionProto->addComponent<explosion_sc>();
	auto pe4 = explosionProto->addComponent<particle_emitter>();
	pe4->setPrototype(emitterProto2);
	auto pe5 = explosionProto->addComponent<particle_emitter>();
	pe5->setPrototype(emitterProto4);

	ExplosionProto = explosionProto;

	////////////////////////////////////////////////

	//gameObjects.front()->addComponent<mvpSolver>();

	proto = CUBE;

	game_object *go = new game_object(*CUBE);
	for (int i = 0; i < n; i++)
	{
		go = new game_object(*go);
		go->transform->translate(randomSphere() * 3.f);
		if (fmod((float)i, (n / 100)) < 0.01)
			cout << "\r" << (float)i / (float)n << "    " << flush;
		go->getComponent<rigidBody>()->setVelocity(randomSphere() * randf() * 100.f);
	}
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
	proto2->addComponent<_renderer>();
	proto2->addComponent<rigidBody>()->gravity = false;
	proto2->addComponent<collider>();
	// proto2->addComponent<cube_sc>();
	proto2->getComponent<_renderer>()->set(modelShader, cubeModel);
	proto2->addComponent<particle_emitter>();
	// proto2->removeComponent<cube_sc>();
	// proto2->removeComponent<particle_emitter>();
	proto2->getComponent<particle_emitter>()->setPrototype(ep2);
	proto2->transform->setScale(glm::vec3(10.f));
	proto2->transform->translate(glm::vec3(10.f) * 0.5f);
	for (int i = 0; i < 15; ++i)
	{
		proto2 = new game_object(*proto2);
		proto2->transform->setScale(glm::vec3(pow(10.f, (float)(i + 1))));
		proto2->transform->translate(glm::vec3(pow(10.f, (float)(i + 1))) * 2.f);
	}

	run();

	return 0;
}
