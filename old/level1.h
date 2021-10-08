#include "game_engine.h"
// #include "initMain.h"
#include <fstream>
#include <iostream>
#include <string>

#include <iomanip>
#include <locale>
// #include "bullet/src/BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h"
#include <bitset>
#include "editor.h"

terrain *terr;
int numBoxes = 0;

audio gunSound;
game_object *physObj;
// int colCount = 0;

using namespace glm;

// game_object *proto = nullptr;
// game_object *ExplosionProto = nullptr;
emitter_prototype_ _expSmoke;
emitter_prototype_ _expFlame;

// struct bullet
// {
// 	game_object_proto *proto;
// 	emitter_prototype_ primarybullet;
// 	// emitter_prototype_ secondarybullet;
// 	emitter_prototype_ primaryexplosion;
// 	// emitter_prototype_ secondaryexplosion;
// };

// map<string, bullet> bullets;

class missile final : public component
{
public:
	// rigidBody *rb;
	vec3 vel;
	// bullet b;
	audio explosionSound;
	emitter_prototype_ exp;
	float explosion_size;
	bool playSound = false;
	void onEdit()
	{
		RENDER(exp);
		RENDER(explosion_size);
		RENDER(explosionSound);
		RENDER(playSound);
	}
	missile() {}

	// void setBullet(const bullet &_b)
	// {
	// 	b = _b;
	// }
	void update()
	{

		transform->move(vel * Time.deltaTime);
		vel += vec3(0, -9.81, 0) * Time.deltaTime;
	}
	void onCollision(game_object *go, vec3 point, vec3 normal)
	{

		if (length(normal) == 0)
			normal = randomSphere();
		exp.burst(point, normal, transform->getScale() * explosion_size, 5);
		transform->gameObject()->destroy();
		// }
		// numCubes.fetch_add(-1);

		// b.primaryexplosion.burst(transform->getPosition(),normal,transform->getScale(),10);
		if (playSound)
			explosionSound.play(transform->getPosition(), 0.5, 0.05);
		// getEmitterPrototypeByName("shockWave").burst(transform->getPosition(),normal,transform->getScale(),25);
		// getEmitterPrototypeByName("debris").burst(transform->getPosition(),normal,transform->getScale(),7);
		// hit = true;
		// }
	}
	COPY(missile);
	// SER5(vel, exp, explosionSound, explosion_size, playSound);
	SER_HELPER()
	{
		SER_BASE(component);
		ar &vel &exp &explosionSound &explosion_size &playSound;
	}
};
REGISTER_COMPONENT(missile)

class gun final : public component
{
	float lastFire;
	vector<vec3> barrels = {vec3(0, 0, 0)};

public:
	game_object_prototype ammo;
	float rof;
	float speed;
	float dispersion;
	float size = 1;
	void onStart()
	{
		lastFire = Time.time;
	}
	void setBarrels(vector<vec3> b)
	{
		barrels = b;
	}
	bool isReloaded()
	{
		return Time.time - lastFire > 1.f / rof;
	}
	bool fire()
	{

		// reload += rof * Time.deltaTime;
		if ((Time.time - lastFire) * rof > 1.f)
		{
			float x = Time.deltaTime * rof;
			float reload = std::max((float)(int)x, 1.f);
			float r;
			// if(reload > x)
			// 	r = Time.deltaTime * (reload / x);
			// else
			r = Time.deltaTime * (1 - (x - reload));
			lastFire = Time.time - r;
			for (int i = 0; i < (int)reload; i++)
			{
				for (auto &j : barrels)
				{
					game_object *go = instantiate(ammo);
					go->transform->setScale(vec3(size));
					go->transform->setPosition(transform->getPosition() + vec3(toMat4(transform->getRotation()) * scale(transform->getScale()) * vec4(j, 1)));
					// go->getComponent<physicsObject>()->init((transform->forward() + randomSphere() * dispersion) * speed);
					go->getComponent<missile>()->vel = (transform->forward() + randomSphere() * dispersion) * speed;
				}
			}
			// numCubes.fetch_add((int)reload);
			// reload -= (int)reload;
			return true;
		}
		return false;
	}
	// //UPDATE(gun, update);
	void onEdit()
	{
		RENDER(rof)
		RENDER(speed)
		RENDER(dispersion)
		RENDER(barrels)
		// if (ImGui::TreeNode("barrels"))
		// {
		// 	for (int i{0}; i < barrels.size(); ++i)
		// 	{
		// 		renderEdit(to_string(i).c_str(), barrels[i]);
		// 	}
		// 	ImGui::TreePop();
		// }
	}
	SER5(rof, speed, dispersion, barrels, ammo);
};
REGISTER_COMPONENT(gun)

float ship_accel;
float ship_vel;

// class nbody : public component
// {
// 	//    glm::vec3 vel;
// 	rigidBody *rb;

// public:
// 	void onStart()
// 	{
// 		rb = transform->gameObject()->getComponent<rigidBody>();
// 	}
// 	void update()
// 	{
// 		deque<nbody> &v = COMPONENT_LIST(nbody)->data.data;
// 		glm::vec3 acc(0);
// 		//        cout << v.size();
// 		for (auto &i : v)
// 		{
// 			glm::vec3 dir = i.transform->getPosition() - transform->getPosition();
// 			float r = glm::length2(dir);
// 			if (r < 0.1f || r > 5000.f)
// 				continue;
// 			if (!vecIsNan(dir))
// 				acc += 10.f * glm::normalize(dir) / r;
// 		}
// 		if (vecIsNan(acc * Time.deltaTime))
// 		{
// 			acc = glm::vec3(0);
// 		}
// 		rb->setVelocity(rb->getVelocity() + acc * Time.deltaTime);
// 		//        transform->move(vel * Time.deltaTime);
// 	}
// 	//UPDATE(nbody, update);
// 	COPY(nbody);
// };
class spinner final : public component
{
public:
	vec3 axis;
	float speed{0.1f};
	void update()
	{
		transform->rotate(axis, speed * Time.deltaTime);
	}
	void onStart()
	{
		axis = normalize(randomSphere());
	}
	void onEdit()
	{
		RENDER(axis);
		RENDER(speed);
	}
	//UPDATE(spinner,update);
	COPY(spinner);
	SER2(speed, axis);
};
REGISTER_COMPONENT(spinner)

const float _pi = radians(180.f);
class _turret final : public component
{
	transform2 target;
	transform2 guns;
	// emitter_prototype_ muzzelSmoke;
	gun *barrels = 0;
	// audiosource *sound = 0;
	float turret_angle;
	float guns_angle;
	bool canFire;

public:
	emitter_prototype_ muzzelFlash;
	float turret_speed = radians(30.f);
	float gun_speed = radians(30.f);
	float t_angles[3];
	float g_angles[3][2];
	// bool forward;
	// bool under;
	void setTarget(transform2 t)
	{
		target = t;
	}
	float getRateOfFire()
	{
		if (barrels == 0)
		{
			if(guns.id == -1)
				this->onStart();
			barrels = guns->gameObject()->getComponent<gun>();
		}
		return barrels->rof;
	}
	void onStart()
	{
		// vec3 up = transform->getParent()->up();
		// if(under){
		// 	up = -up;
		// }
		// if(forward){
		// 	transform->lookat(transform->getParent()->forward(),up);
		// }else{
		// 	transform->lookat(-transform->getParent()->forward(),up);
		// }
		guns = transform->getChildren().front();
		barrels = guns->gameObject()->getComponent<gun>();
		// sound = transform->gameObject()->getComponent<audiosource>();
		// sound->gain = 0.05;
		muzzelFlash = getNamedEmitterProto("muzzelFlash");
		// muzzelSmoke = getEmitterPrototypeByName("muzzelSmoke");
	}
	void update()
	{
		/////////////////////////////////////////////// turn turret
		vec3 targetPos = inverse(toMat3(transform->getRotation())) * (target->getPosition() - transform->getPosition());
		targetPos = normalize(vec3(targetPos.x, 0, targetPos.z));
		float angle = acos(targetPos.z);
		float turn_angle = std::min(turret_speed * Time.deltaTime, angle) * (targetPos.x > 0 ? 1 : -1);
		angle *= (targetPos.x > 0 ? 1 : -1);

		if ((turret_angle + angle > 0 && turret_angle + angle - _pi > 0) || (turret_angle + angle < 0 && turret_angle + angle + _pi < 0))
		{
			turn_angle = turret_speed * Time.deltaTime * (targetPos.x > 0 ? -1 : 1);
		}
		if (turret_angle + turn_angle > t_angles[2])
		{ // left max angle
			turn_angle = t_angles[2] - turret_angle;
		}
		else if (turret_angle + turn_angle < t_angles[0])
		{ // right max angle
			turn_angle = t_angles[0] - turret_angle;
		}

		canFire = abs(angle) < 0.01; //turret_speed * Time.deltaTime;
		turret_angle += turn_angle;
		float turret_turn_angle = turn_angle - angle;
		transform->rotate(vec3(0, 1, 0), angle);

		/////////////////////////////////////////////// turn guns
		targetPos = inverse(toMat3(guns->getRotation())) * (target->getPosition() - transform->getPosition());
		targetPos = normalize(vec3(0, targetPos.y, targetPos.z));
		angle = acos(targetPos.z);
		turn_angle = std::min(gun_speed * Time.deltaTime, angle) * (targetPos.y > 0 ? -1 : 1);
		angle *= (targetPos.y > 0 ? 1 : -1);

		int index = (turret_angle > 0 ? 2 : 0);
		float ratio = abs(turret_angle / t_angles[index]);
		float guns_angles[2] = {g_angles[index][0] * ratio + g_angles[1][0] * (1 - ratio),
								g_angles[index][1] * ratio + g_angles[1][1] * (1 - ratio)};

		// if((guns_angle + angle > 0 && guns_angle + angle - pi > 0) || (guns_angle + angle < 0 && guns_angle + angle + pi < 0)){
		// 	turn_angle = gun_speed * Time.deltaTime * (targetPos.x > 0 ? -1 : 1);
		// }
		if (guns_angle + turn_angle > guns_angles[1])
		{ // up max angle
			turn_angle = guns_angles[1] - guns_angle;
		}
		else if (guns_angle + turn_angle < guns_angles[0])
		{ // down max angle
			turn_angle = guns_angles[0] - guns_angle;
		}

		guns_angle += turn_angle;
		guns->rotate(vec3(1, 0, 0), turn_angle);

		transform->rotate(vec3(0, 1, 0), turret_turn_angle);

		canFire = canFire && abs(angle) < 0.01; //gun_speed * Time.deltaTime;
												// if(canFire && Input.Mouse.getButton(GLFW_MOUSE_BUTTON_LEFT)){
												// 	if(barrels->fire()){
												// 		// cout << "fire" << endl;
												// 		muzzelFlash.burst(guns->forward() * guns->getScale() * 5.3f + guns->getPosition(),guns->forward(),20);
												// 		getEmitterPrototypeByName("shockWave").burst(transform->getPosition(),guns->forward(),vec3(0.2),60);

		// 		// muzzelSmoke.burst(guns->forward() * guns->getScale() * 5.3f + guns->getPosition(),guns->forward(),17);
		// 	}
		// }
	}
	bool onTarget()
	{
		return canFire;
	}
	bool reloaded()
	{
		return barrels->isReloaded();
	}
	bool fire()
	{
		if (canFire)
		{
			if (barrels->fire())
			{
				// cout << "fire" << endl;
				muzzelFlash.burst(guns->forward() * guns->getScale() * 5.3f + guns->getPosition(), guns->forward(), 20);
				getNamedEmitterProto("shockWave").burst(transform->getPosition(), guns->forward(), vec3(0.2), 60);
				// sound->play();
				// muzzelSmoke.burst(guns->forward() * guns->getScale() * 5.3f + guns->getPosition(),guns->forward(),17);
				return true;
			}
		}
		return false;
	}
	void onEdit()
	{
		RENDER(turret_speed);
		RENDER(gun_speed);
		RENDER(muzzelFlash);
		RENDER(target);
	}
	//UPDATE(_turret,update);
	COPY(_turret);
	SER_HELPER()
	{
		SER_BASE(component);
		ar &target &gun_speed &turret_speed &muzzelFlash &guns &t_angles &g_angles &turret_angle &guns_angle; // & forward & under;
	}
};
REGISTER_COMPONENT(_turret)

class gunManager final : public component
{

	vector<_turret *> turrets;
	int curr = 0;
	double t;
	double t_;

public:
	void fire()
	{
		if (Time.time > t)
		{
			for (auto &i : turrets)
			{
				if (i->reloaded() && i->onTarget())
				{
					i->fire();
					t = Time.time + t_;
					curr = (curr + 1) % turrets.size();
					break;
				}
			}
		}
	}
	void onStart()
	{
		for (auto &i : transform->getChildren())
		{
			auto tur = i->gameObject()->getComponent<_turret>();
			if (tur != 0)
			{
				turrets.push_back(tur);
			}
		}
		rof = (1.f / turrets[0]->getRateOfFire()) / turrets.size();
	}
	void reset(){
		turrets.clear();
		onStart();
	}

public:
	void onEdit() {
		if(ImGui::Button("reset")){
			reset();
		}
	}
	//UPDATE(gunManager,update);
	COPY(gunManager);
	SER0();
};
REGISTER_COMPONENT(gunManager)

class autoShooter final : public component
{
public:
	bool shouldFire;
	gun *g;
	void onStart()
	{
		g = transform->gameObject()->getComponent<gun>();
	}
	void update()
	{
		if (Input.getKey(GLFW_KEY_H))
			shouldFire = false;
		else if (Input.getKey(GLFW_KEY_J))
			shouldFire = true;
		if (shouldFire)
			g->fire();
	}
	void onEdit() {}
	//UPDATE(autoShooter, update);
	COPY(autoShooter);
	SER1(shouldFire);
};
REGISTER_COMPONENT(autoShooter)

class _ship : public component
{
	vec3 vel;

public:
	float accel;
	float thrust;
	float maxReverse;
	float maxForward;
	float rotationSpeed;
	// void onStart()
	// {
	// 	accel = 0;
	// 	thrust = 20;
	// 	maxReverse = 50;
	// 	maxForward = 10000;
	// 	rotationSpeed = radians(10.f);
	// }
	void accelerate(float acc)
	{

		// if(length(vel) > maxForward){
		// 	vel = normalize(vel) * maxForward;
		// }
		accel = glm::clamp(accel + acc * thrust * Time.deltaTime, -maxReverse, maxForward);
	}
	void maxAccel()
	{
		accel = maxForward;
	}
	void stop()
	{
		accel = 0;
	}
	void pitch(float _pitch)
	{
		transform->rotate(glm::vec3(1, 0, 0), _pitch * Time.deltaTime * rotationSpeed);
	}
	void yaw(float _yaw)
	{
		transform->rotate(glm::vec3(0, 1, 0), _yaw * Time.deltaTime * rotationSpeed);
	}
	void roll(float _roll)
	{
		transform->rotate(glm::vec3(0, 0, 1), _roll * Time.deltaTime * rotationSpeed);
	}
	void update()
	{
		vel -= vel * 0.4f * Time.deltaTime;
		vel += transform->forward() * accel * 0.4f * Time.deltaTime;
		accel = glm::clamp(accel, -maxReverse, maxForward);
		if(length(vel) > maxForward){
			vel = normalize(vel) * maxForward;
		}
		transform->getParent()->move(vel * Time.deltaTime, true);
		ship_vel = length(vel);
		ship_accel = accel;
		// 	transform->rotate(glm::vec3(0, 1, 0), (Input.getKey(GLFW_KEY_A) - Input.getKey(GLFW_KEY_D)) * Time.deltaTime * rotationSpeed);
		// 	transform->rotate(glm::vec3(1, 0, 0), (Input.getKey(GLFW_KEY_W) - Input.getKey(GLFW_KEY_S)) * Time.deltaTime * rotationSpeed);
		// 	transform->rotate(glm::vec3(0, 0, 1), (Input.getKey(GLFW_KEY_SPACE) - Input.getKey(GLFW_KEY_LEFT_SHIFT)) * Time.deltaTime * rotationSpeed);

		// 	vel -= vel * 0.4f * Time.deltaTime;
		// 	vel += transform->forward() * accel * 0.4f * Time.deltaTime;
		// 	ship_vel = length(vel);
		// 	ship_accel = accel;
		// 	// if(length(vel) > maxForward){
		// 	// 	vel = normalize(vel) * maxForward;
		// 	// }
		// 	accel = glm::clamp(accel + (Input.getKey(GLFW_KEY_R) - Input.getKey(GLFW_KEY_F)) * thrust * Time.deltaTime, -maxReverse, maxForward);
		// 	// cout << " accel: " << accel << endl;
		// 	transform->getParent()->move(vel * Time.deltaTime, true);
		// 	if (Input.getKey(GLFW_KEY_T))
		// 	{
		// 		accel = maxForward;
		// 	}
		// 	if (Input.getKey(GLFW_KEY_G))
		// 	{
		// 		accel = 0;
		// 	}
		// 	// vec3 inputVel = ((float)(Input.getKey(GLFW_KEY_A) - Input.getKey(GLFW_KEY_D)) * transform->right()
		// 	//  + (float)(Input.getKey(GLFW_KEY_SPACE) - Input.getKey(GLFW_KEY_LEFT_SHIFT)) * transform->up()
		// 	//  + (float)(Input.getKey(GLFW_KEY_W) - Input.getKey(GLFW_KEY_S)) * transform->forward());
	}

	void onCollision(game_object *go, vec3 point, vec3 normal)
	{
		getNamedEmitterProto("shockWave").burst(point, transform->forward(), vec3(0.5), 25);
	}
	void onEdit()
	{
		// RENDER(accel);
		RENDER(thrust);
		RENDER(maxReverse);
		RENDER(maxForward);
		RENDER(rotationSpeed);
	}
	//UPDATE(_ship,update);
	COPY(_ship);
	SER4(maxReverse, maxForward,thrust,rotationSpeed);
};
REGISTER_COMPONENT(_ship)

class _boom : public component
{
public:
	transform2 t;
	void update()
	{
		transform->setPosition(t->getPosition());
	}
	void onEdit()
	{
		RENDER(t.id);
	}
	//UPDATE(_boom,update);
	COPY(_boom);
	SER1(t);
};
REGISTER_COMPONENT(_boom)

class player_sc : public component
{
	bool cursorReleased = false;
	float speed = 10.f;
	// rigidBody *rb;
	bool flying = true;
	bool jumped = false; // do not fly and jump in same frame
	int framecount = 0;
	vec3 ownSpeed = vec3(0);
	// bullet bomb;
	// bullet laser;
	vector<gun *> guns;
	float rotationSpeed = 10.f;
	float rotX;
	float rotY;
	float fov;
	_camera *cam;
	_ship *ship;
	gunManager *gm;

	// gui::window *info;
	// gui::text *fps;
	// gui::text *missileCounter;
	// gui::text *particleCounter;
	// gui::text *shipAcceleration;
	// gui::text *shipVelocity;
	// gui::text *lockedfrustum;
	// gui::text *colCounter;
	// gui::window *reticule;
	// gui::image *crosshair;
	// _texture crosshairtex;
	game_object_prototype ammo_proto;

public:
	terrain *t;
	void onEdit()
	{
		RENDER(speed);
	}

	// void onCollision(game_object *collidee)
	// {
	// 	colliding = true;
	// }

	void onStart()
	{
		list<transform2> vt = transform->getParent()->getParent()->getChildren();
		transform2 ship_t;
		for (auto &i : vt)
		{
			if (i->name() == "ship")
				ship_t = i;
		}

		ship = ship_t->gameObject()->getComponent<_ship>();
		gm = ship_t->gameObject()->getComponent<gunManager>();
		// rb = transform->gameObject()->getComponent<rigidBody>();
		guns = transform->gameObject()->getComponents<gun>();
		// bomb = bullets["bomb"];
		guns[0]->ammo = ammo_proto;
		guns[0]->rof = 3'000 / 60;
		guns[0]->dispersion = 0.3f;
		guns[0]->speed = 200;
		// laser = bullets["laser"];
		// guns[1]->ammo = bullets["laser"].proto;
		guns[1]->rof = 1000 / 60;
		guns[1]->dispersion = 0;
		guns[1]->speed = 30000;
		guns[1]->size = 20;
		guns[0]->setBarrels({vec3(0.f, -10.f, 45.f)});

		// info = new gui::window();
		// fps = new gui::text();
		// missileCounter = new gui::text();
		// particleCounter = new gui::text();
		// shipAcceleration = new gui::text();
		// shipVelocity = new gui::text();
		// lockedfrustum = new gui::text();
		// colCounter = new gui::text();
		// info->name = "game info";
		// ImGuiWindowFlags flags = 0;
		// flags |= ImGuiWindowFlags_NoTitleBar;
		// flags |= ImGuiWindowFlags_NoMove;
		// flags |= ImGuiWindowFlags_NoResize;
		// // flags |= ImGuiWindowFlags_NoBackground;
		// info->flags = flags;
		// info->pos = ImVec2(20, 20);
		// info->size = ImVec2(200, 150);
		// info->children.push_back(fps);
		// info->children.push_back(missileCounter);
		// info->children.push_back(particleCounter);
		// info->children.push_back(lockedfrustum);
		// info->children.push_back(shipAcceleration);
		// info->children.push_back(shipVelocity);
		// info->adopt(colCounter);
		// reticule = new gui::window();
		// flags |= ImGuiWindowFlags_NoBackground;
		// flags |= ImGuiWindowFlags_NoScrollbar;
		// flags &= ~ImGuiWindowFlags_NoMove;
		// reticule->flags = flags;
		// reticule->name = "reticule";
		// reticule->pos = ImVec2(0, 0);
		// crosshair = new gui::image();
		// waitForRenderJob([&]() { crosshairtex.load("res/images/crosshair.png"); });
		// crosshair->img = crosshairtex;
		// reticule->adopt(crosshair);
		// glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		cam = transform->gameObject()->getComponent<_camera>();
		fov = cam->c->fov;
	}
	void update()
	{

		// rb->gravity = false;
		float _80 = radians(80.f);
		transform->getParent()->rotate(inverse(transform->getParent()->getRotation()) * vec3(0, 1, 0), Input.Mouse.getX() * Time.unscaledDeltaTime * rotationSpeed * fov / _80 * -0.01f);
		transform->getParent()->rotate(vec3(1, 0, 0), Input.Mouse.getY() * Time.unscaledDeltaTime * rotationSpeed * fov / _80 * -0.01f);

		// transform->translate(vec3(0,1,-4) * -Input.Mouse.getScroll());
		fov -= Input.Mouse.getScroll() * 0.5;
		fov = glm::clamp(fov, radians(5.f), _80);
		transform->gameObject()->getComponent<_camera>()->c->fov = fov; //Input.Mouse.getScroll();

		if (framecount++ > 1)
		{

			// fps->contents = "fps: " + to_string(1.f / Time.unscaledSmoothDeltaTime);
			// missileCounter->contents = "missiles: " + FormatWithCommas(COMPONENT_LIST(missile)->active());
			// particleCounter->contents = "particles: " + FormatWithCommas(getParticleCount());
			// shipVelocity->contents = "speed: " + to_string(ship_vel);
			// shipAcceleration->contents = "thrust: " + to_string(ship_accel);
			// lockedfrustum->contents = "locked frustum: " + to_string(transform->gameObject()->getComponent<_camera>()->c->lockFrustum);
			// reticule->size = ImVec2(SCREEN_WIDTH, SCREEN_HEIGHT);
			// crosshair->pos = ImVec2(SCREEN_WIDTH / 2 - 240, SCREEN_HEIGHT / 2 - 200);
			// colCounter->contents = "collisions: " + to_string(colCount);
		}
		// cout << "\rmissiles: " + FormatWithCommas(FormatWithCommas(COMPONENT_LIST(missile)->active())) + "       ";

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

		if (Input.getKeyDown(GLFW_KEY_M) && cursorReleased)
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			cursorReleased = false;
		}
		else if (Input.getKeyDown(GLFW_KEY_M) && !cursorReleased)
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			cursorReleased = true;
		}
		if (Input.Mouse.getButton(GLFW_MOUSE_BUTTON_LEFT))
		{

			// 	for(int i = 0; i <= Time.deltaTime * 100; i++){
			// 		numBoxes++;
			// 		auto g = new game_object(*physObj);
			// 		vec3 r = randomSphere() * 2.f * randf() + transform->getPosition() + transform->forward() * 12.f;
			// 		physObj->getComponent<physicsObject>()->init(r.x,r.y,r.z, transform->forward() * 30.f + randomSphere()*10.f);
			// 	}
			// guns[0]->fire();
			gm->fire();
		}
		// if (Input.Mouse.getButton(GLFW_MOUSE_BUTTON_RIGHT))
		// {
		// 	// for(int i = 0; i <= Time.deltaTime * 100; i++){
		// 	// 	numBoxes++;
		// 	// 	auto g = new game_object(*physObj);
		// 	// 	vec3 r = randomSphere() * 2.f * randf() + transform->getPosition() + transform->forward() * 12.f;
		// 	// 	physObj->getComponent<physicsObject>()->init(r.x,r.y,r.z, transform->forward() * 30.f + randomSphere()*10.f);
		// 	// }
		// 	guns[1]->fire();
		// }
		// 	transform->rotate(glm::vec3(0, 1, 0), (Input.getKey(GLFW_KEY_A) - Input.getKey(GLFW_KEY_D)) * Time.deltaTime * rotationSpeed);
		// 	transform->rotate(glm::vec3(1, 0, 0), (Input.getKey(GLFW_KEY_W) - Input.getKey(GLFW_KEY_S)) * Time.deltaTime * rotationSpeed);
		// 	transform->rotate(glm::vec3(0, 0, 1), (Input.getKey(GLFW_KEY_SPACE) - Input.getKey(GLFW_KEY_LEFT_SHIFT)) * Time.deltaTime * rotationSpeed);
		ship->pitch(Input.getKey(GLFW_KEY_W) - Input.getKey(GLFW_KEY_S));
		ship->roll(Input.getKey(GLFW_KEY_SPACE) - Input.getKey(GLFW_KEY_LEFT_SHIFT));
		ship->yaw(Input.getKey(GLFW_KEY_A) - Input.getKey(GLFW_KEY_D));
		ship->accelerate(Input.getKey(GLFW_KEY_R) - Input.getKey(GLFW_KEY_F));
	}
	//UPDATE(player_sc, update);
	COPY(player_sc);
	SER1(speed);
};
REGISTER_COMPONENT(player_sc)


class player_sc3 : public component
{
public:
	float speed = 3.f;
	bool cursorReleased = false;
	float fov = 80;
	gui::window *info;
	gui::text *fps;
	vector<gun *> guns;
	game_object_prototype ammo_proto;

	void onStart()
	{
		info = new gui::window();
		fps = new gui::text();
		info->adopt(fps);
		info->name = "game info";
		// ImGuiWindowFlags flags = 0;
		// flags |= ImGuiWindowFlags_NoTitleBar;
		// flags |= ImGuiWindowFlags_NoMove;
		// flags |= ImGuiWindowFlags_NoResize;
		// info->flags = flags;
		info->pos = ImVec2(20, 20);
		info->size = ImVec2(200, 150);

		guns = transform->gameObject()->getComponents<gun>();
		// bomb = bullets["bomb"];
		guns[0]->ammo = ammo_proto; //bullets["bomb"].proto;
		guns[0]->rof = 3'000 / 60;
		guns[0]->dispersion = 0.3f;
		guns[0]->speed = 200;
		guns[0]->setBarrels({vec3(-1.1, 0.4, 0.5)});
	}
	void update()
	{
		fps->contents = "fps: " + to_string(1.f / Time.unscaledSmoothDeltaTime);

		transform->translate(glm::vec3(1, 0, 0) * (float)(Input.getKey(GLFW_KEY_A) - Input.getKey(GLFW_KEY_D)) * Time.deltaTime * speed);
		transform->translate(glm::vec3(0, 0, 1) * (float)(Input.getKey(GLFW_KEY_W) - Input.getKey(GLFW_KEY_S)) * Time.deltaTime * speed);
		transform->rotate(vec3(0, 1, 0), Input.Mouse.getX() * Time.unscaledDeltaTime * fov / 80 * -0.4f);
		transform->rotate(vec3(1, 0, 0), Input.Mouse.getY() * Time.unscaledDeltaTime * fov / 80 * -0.4f);
		vec3 pos = transform->getPosition();
		pos.y = getTerrain(pos.x, pos.z)->getHeight(pos.x, pos.z).height + 1.8;
		transform->setPosition(pos);
		// transform->setRotation(quatLookAtLH(transform->getRotation() * vec3(0,0,1),vec3(0,1,0)));
		transform->lookat(transform->forward(), vec3(0, 1, 0));

		fov -= Input.Mouse.getScroll() * 5;
		fov = glm::clamp(fov, 5.f, 80.f);
		transform->gameObject()->getComponent<_camera>()->c->fov = fov; //Input.Mouse.getScroll();

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
		if (Input.getKeyDown(GLFW_KEY_R))
		{
			speed *= 2;
		}
		if (Input.getKeyDown(GLFW_KEY_F))
		{
			speed /= 2;
		}
		if (Input.Mouse.getButton(GLFW_MOUSE_BUTTON_LEFT))
		{
			guns[0]->fire();
		}
	}
	void onEdit() {}
	COPY(player_sc3);
	SER3(speed, cursorReleased, fov);
};
REGISTER_COMPONENT(player_sc3)
class sun_sc : public component
{
	COPY(sun_sc);
	SER2(distance, day_cycle);

public:
	float distance = 30'000;
	float day_cycle = 30;
	void update()
	{
		transform->setPosition(root2.getPosition() + vec3(cos(Time.time / day_cycle), sin(Time.time / day_cycle), 0) * distance * mat3(rotate(radians(45.f), vec3(0, 0, 1))));
	}
	void onEdit()
	{
		RENDER(distance);
		RENDER(day_cycle);
	}
};
REGISTER_COMPONENT(sun_sc)

void makeGun(transform2 ship, vec3 pos, transform2 target, bool forward, bool upright, game_object_prototype ammo_proto)
{
	// _shader wireFrame("res/shaders/wireframe.vert","res/shaders/wireframe.geom","res/shaders/wireframe.frag");
	_shader modelShader("res/shaders/model.vert", "res/shaders/model.frag");
	_model turretm("res/models/ship1/maingun.obj");
	_model gunsm("res/models/ship1/3guns.obj");

	game_object *turret = _instantiate();
	// turret->_addComponent<audiosource>()->set(gunSound);
	auto r = turret->_addComponent<_renderer>();
	r->set(modelShader, turretm);
	game_object *guns = _instantiate();
	r = guns->_addComponent<_renderer>();
	r->set(modelShader, gunsm);
	turret->transform->adopt(guns->transform);
	guns->transform->translate(vec3(0, -0.1, 0.6));
	vector<vec3> barrels = {vec3(-.56, 0, 2.3), vec3(0, 0, 2.3), vec3(0.56, 0, 2.3)};
	auto g = guns->_addComponent<gun>();
	g->setBarrels(barrels);
	g->ammo = ammo_proto; //bullets["bomb"].proto;
	g->rof = 1.f / 4.f;
	g->dispersion = 0.01f;
	g->speed = 500;

	ship->adopt(turret->transform);
	turret->transform->translate(pos);

	if (!upright)
	{
		turret->transform->rotate(vec3(0, 0, 1), radians(180.f));
	}
	if (!forward)
	{
		turret->transform->rotate(vec3(0, 1, 0), radians(180.f));
	}
	auto t = turret->_addComponent<_turret>();
	// t->forward = forward;
	// t->under = upright;
	t->setTarget(target);
	t->t_angles[0] = radians(-135.f);
	t->t_angles[1] = radians(0.f);
	t->t_angles[2] = radians(135.f);

	t->g_angles[0][0] = radians(-80.f);
	t->g_angles[0][1] = radians(20.f);
	t->g_angles[1][0] = radians(-80.f);
	t->g_angles[1][1] = radians(3.f);
	t->g_angles[2][0] = radians(-80.f);
	t->g_angles[2][1] = radians(20.f);
	t->turret_speed = glm::radians(100.f);
	t->gun_speed = glm::radians(100.f);
}

#define REG_ASSET(_name)         \
	_name.meta()->name = #_name; \
	assets::registerAsset(_name.meta());

#define REG_ASSET2(_name) \
	_name.name = #_name;  \
	assets::registerAsset(&_name);

int level1(bool load)
{

	seedRand(vec3(123456789, 345678901, 567890123));
	genNoise(512, 512, 4);

	physics_manager::collisionGraph[-1] = {};
	physics_manager::collisionGraph[0] = {1};
	physics_manager::collisionGraph[1] = {0, 1};

	if (true)
	{
		gunSound = audio("res/audio/explosion1.wav");
		gunSound.meta()->name = "gunSound";
		assets::registerAsset(gunSound.meta());
		_shader modelShader("res/shaders/model.vert", "res/shaders/model.frag");
		modelShader.meta()->name = "modelShader";
		assets::registerAsset(modelShader.meta());
		_shader lampShader("res/shaders/model.vert", "res/shaders/lamp.frag");
		lampShader.meta()->name = "lampShader";
		assets::registerAsset(lampShader.meta());

		// _shader terrainShader("res/shaders/model.vert", "res/shaders/terrain.frag");
		_shader terrainShader("res/shaders/terrainShader/terrain.vert",
							  "res/shaders/terrainShader/terrain.tesc",
							  "res/shaders/terrainShader/terrain.tese",
							  //   "res/shaders/terrainShader/terrain.geom",
							  "res/shaders/terrainShader/terrain.frag");
		terrainShader.meta()->shader->primitiveType = GL_PATCHES;
		terrainShader.meta()->name = "terrainShader";
		assets::registerAsset(terrainShader.meta());

		_shader wireFrame2("res/shaders/wireframe.vert", "res/shaders/wireframe.geom", "res/shaders/wireframe.frag");
		wireFrame2.meta()->name = "wireFrameShader";
		assets::registerAsset(wireFrame2.meta());

		_model cubeModel("res/models/cube/cube.obj");
		REG_ASSET(cubeModel);
		_model nanoSuitModel("res/models/nanosuit/nanosuit.obj");
		REG_ASSET(nanoSuitModel);
		_model terrainModel("res/models/terrain/terrain.obj");
		REG_ASSET(terrainModel);
		_model treeModel("res/models/Spruce_obj/Spruce.obj");
		REG_ASSET(treeModel);

		// while (!cubeModel.meta()->model->ready())
		// 	this_thread::yield();
		// boxPoints = cubeModel.meta()->model->meshes[0].vertices;
		// boxTris = cubeModel.meta()->model->meshes[0].indices;

		colorArray ca;
		ca.addKey(vec4(1), 0.03)
			.addKey(vec4(1, 1, 0.5f, 1), 0.05)
			.addKey(vec4(1, 0.8f, 0.5f, 0.9f), 0.07)
			// .addKey(vec4(1,0.6f,0.5f,0.9f),0.09)
			.addKey(vec4(0.65, 0.65, 0.65, 0.3f), 0.09)
			.addKey(vec4(0.65, 0.65, 0.65, 0.0f), 8.f / 9.f);
		addColorArray(ca);

		colorArray ca3;
		ca3.addKey(vec4(1), 0.1)
			.addKey(vec4(1, 1, 0.9f, 1), 0.15)
			.addKey(vec4(1, 0.8f, 0.5f, 0.9f), 0.2)
			.addKey(vec4(1, 0.5f, 0.5f, 0.9f), 0.25)
			.addKey(vec4(0.75, 0.65, 0.54, 0.6f), 0.30)
			.addKey(vec4(0.75, 0.65, 0.54, 0.0f), 1.f);
		addColorArray(ca3);
		colorArray ca2;
		ca2.addKey(vec4(1), 0.05)
			.addKey(vec4(1, 1, 0.9f, 1), 0.09)
			.addKey(vec4(1, 0.8f, 0.5f, 0.9f), 0.12)
			.addKey(vec4(1, 0.5f, 0.5f, 0.9f), 0.17)
			.addKey(vec4(0.5, 0.5f, 0.5f, 0.6f), 0.21)
			.addKey(vec4(0.5, 0.5f, 0.5f, 0.0f), 1);
		addColorArray(ca2);
		floatArray fa;
		fa.addKey(0.f, 0.f).addKey(0.4f, 0.02f).addKey(2.0, 1.0);

		emitter_prototype_ flameEmitterProto = createNamedEmitter("flame");
		REG_ASSET(flameEmitterProto);

		flameEmitterProto->dispersion = 3.14159f;
		flameEmitterProto->emission_rate = 1.2f;
		flameEmitterProto->lifetime = 3.f;
		flameEmitterProto->lifetime2 = 3.f;
		flameEmitterProto.size(1.f);
		// flameEmitterProto->color(vec4(1, 1, 0.1f, 1.f));
		flameEmitterProto->maxSpeed = 1.f;
		flameEmitterProto->scale = vec3(1.f);
		flameEmitterProto->billboard = 1;
		flameEmitterProto->trail = 1;
		flameEmitterProto.color(ca);
		// ca.setColorArray(flameEmitterProto->colorLife);

		emitter_prototype_ _muzzelFlash = createNamedEmitter("muzzelFlash");
		REG_ASSET(_muzzelFlash);
		_muzzelFlash->dispersion = 0.5f;
		_muzzelFlash->emission_rate = 1.f;
		_muzzelFlash->lifetime = 4.f;
		_muzzelFlash->lifetime2 = 2.f;
		// _muzzelFlash->color(vec4(1, 1, 0.2f, 0.8f));
		_muzzelFlash->maxSpeed = (10.f);
		_muzzelFlash.size(0.5f, 1.f);
		_muzzelFlash->scale = vec3(7.f);
		_muzzelFlash->trail = 0;
		_muzzelFlash.color(ca2);
		// ca2.setColorArray(_muzzelFlash->colorLife);

		emitter_prototype_ expFlame = createNamedEmitter("expflame");
		REG_ASSET(expFlame);
		expFlame->dispersion = 3.14159f / 2.f;
		expFlame->emission_rate = 50.f;
		expFlame->lifetime = 2.2f;
		expFlame->lifetime2 = 1.5f;
		expFlame->maxSpeed = 30.f;
		expFlame->scale = vec3(30.f);
		expFlame.size(0.5f, 1.6f);
		// fa.setFloatArray(expFlame->sizeLife);
		expFlame->trail = 0;
		_expFlame = expFlame;
		_expFlame.color(ca2);
		// ca2.setColorArray(expFlame->colorLife);

		emitter_prototype_ shockWave = createNamedEmitter("shockWave");
		REG_ASSET(shockWave)
		shockWave->dispersion = 3.14159f / 2.f;
		// shockWave->emission_rate = 50.f;
		shockWave->lifetime = 0.3f;
		shockWave->lifetime2 = 0.1f;
		shockWave->maxSpeed = 300.f;
		shockWave->minSpeed = 250.f;
		shockWave->scale = vec3(40.f);
		shockWave.size(0.5f, 1.f);
		shockWave->trail = 0;
		shockWave.color(vec4(1, 1, 1, 0.6), vec4(1, 1, 1, 0));

		emitter_prototype_ debris = createNamedEmitter("debris");
		REG_ASSET(debris)
		debris->dispersion = 3.14159f / 2.f;
		// debris->emission_rate = 50.f;
		debris->lifetime = 5.f;
		debris->lifetime2 = 3.f;
		debris->maxSpeed = 15.f;
		debris->minSpeed = 10.f;
		debris->radius = 10.f;
		debris->scale = vec3(5, 60, 0) * 2.f;
		debris.size(0.5f, 1.f);
		debris->velAlign = 1;
		debris.color(ca3);
		// debris->color(vec4(0.82,0.7,0.54,0.6),vec4(0.82,0.7,0.54,0));
		// ca3.setColorArray(debris->colorLife);

		// bullet laser;
		// laser.primarybullet = createNamedEmitter("laserbeam");
		// laser.primarybullet->dispersion = 3.14159f;
		// laser.primarybullet->color(vec4(.8, .8, 1, 1), vec4(.6, .6, 1, 0.0));
		// laser.primarybullet->lifetime = 0.3f;
		// laser.primarybullet->lifetime2 = 0.3f;
		// laser.primarybullet->emission_rate = 20.f;
		// laser.primarybullet->trail = 1;
		// laser.primarybullet->scale = vec3(20.f);
		// laser.primarybullet->size(1.f);
		// laser.primarybullet->maxSpeed = 1.f;
		// laser.primaryexplosion = getEmitterPrototypeByName("expflame");

		emitter_prototype_ engineTrail = createNamedEmitter("engineTrail");
		REG_ASSET(engineTrail);
		*engineTrail = *flameEmitterProto;
		engineTrail->dispersion = 0.0f;
		engineTrail->emission_rate = 2.f;
		engineTrail->lifetime = 7.f;
		engineTrail->lifetime2 = 7.f;
		engineTrail.color(vec4(0.6f, 0.7f, 1.f, 0.6f), vec4(0.05f, 0.1f, 1.f, 0.0f));
		engineTrail.size(1.f);
		engineTrail->maxSpeed = (0.f);
		engineTrail->scale = vec3(1);
		engineTrail->trail = 1;

		emitter_prototype_ engineFlame = createNamedEmitter("engineFlame");
		REG_ASSET(engineFlame);
		engineFlame->dispersion = 0.5f;
		engineFlame->emission_rate = 15.f;
		engineFlame->lifetime = 4.f;
		engineFlame->lifetime2 = 3.f;
		engineFlame.color(vec4(0.2f, 0.5f, 0.9f, 0.2f), vec4(0.05f, 0.1f, 0.9f, 0.2f));
		engineFlame->maxSpeed = (-3.f);
		engineFlame.size(1.f, 0.f);
		engineFlame->scale = vec3(2.f);
		engineFlame->trail = 0;

		// bullet bomb;
		// bomb.primarybullet = getNamedEmitterProto("flame");
		// bomb.primaryexplosion = getNamedEmitterProto("expflame");

		game_object_proto_ *bomb_proto = new game_object_proto_();
		bomb_proto->name = "bomb";
		bomb_proto->addComponent<_renderer>()->set_proto(modelShader, cubeModel);
		bomb_proto->addComponent<collider>()->setLayer(0);
		bomb_proto->getComponent<collider>()->dim = vec3(0.1f);
		bomb_proto->getComponent<collider>()->setPoint();
		// bomb_proto->_addComponent<physicsObject>();
		// bomb_proto->_addComponent<audiosource>()->set(gunSound);
		bomb_proto->addComponent<particle_emitter>()->protoSetPrototype(flameEmitterProto);
		// bomb.proto = bomb_proto;
		bomb_proto->addComponent<missile>()->exp = getNamedEmitterProto("expflame"); //bomb.primaryexplosion;//setBullet(bomb);
																					 // bullets["bomb"] = bomb;
																					 // ammo = bullets["bomb"].proto;
		bomb_proto->getComponent<missile>()->explosionSound = gunSound;
		registerProto(bomb_proto);

		// game_object_proto *laser_proto = new game_object_proto();
		// laser_proto->_addComponent<collider>()->setLayer(0);
		// laser_proto->getComponent<collider>()->setPoint();
		// laser_proto->_addComponent<particle_emitter>();
		// laser.proto = laser_proto;
		// laser_proto->_addComponent<missile>()->setBullet(laser);
		// bullets["laser"] = laser;

		//////////////////////////////////////////////////////////

		game_object *light = _instantiate(); //new game_object();
		light->transform.name() = "sun";
		light->transform->setScale(vec3(1000));
		light->transform->setPosition(glm::vec3(30000));
		light->_addComponent<Light>()->setColor(glm::vec3(24000));
		light->getComponent<Light>()->setConstant(1.f);
		light->getComponent<Light>()->setlinear(0.000014f);
		light->getComponent<Light>()->setQuadratic(0.000007f);
		// auto sun = light->_addComponent<sun_sc>();
		// sun->distance = 50'000;
		// sun->day_cycle = 100;
		light->_addComponent<_renderer>()->set(lampShader, cubeModel);

		// physObj = new game_object();
		// physObj->_addComponent<_renderer>()->set(modelShader, cubeModel);
		// physObj->_addComponent<physicsObject>()->init(vec3(0));
		// numBoxes += 61;
		// for(int i = 0; i < 60; i++){
		// 	auto g = new game_object(*physObj);
		// 	vec3 r = randomSphere() * 500.f * randf() + vec3(0,500,0);
		// 	g->transform->setPosition(r);
		// 	g->getComponent<physicsObject>()->init(vec3(0));
		// }

		game_object *player =_instantiate();
		player->transform.name() = "player";
		auto playerCam = player->_addComponent<_camera>();
		playerCam->c->fov = radians(80.f);
		playerCam->c->farPlane = 1e32f;
		playerCam->c->nearPlane = 0.00001f;
		player->_addComponent<gun>();
		player->_addComponent<gun>();
		// player->_addComponent<editor_sc>()->ammo_proto = game_object_prototype(bomb_proto);

		// player->_addComponent<Light>()->setColor(glm::vec3(3, 3, 20));
		// player->getComponent<Light>()->setConstant(1.f);
		// player->getComponent<Light>()->setlinear(0.0005f);
		// player->getComponent<Light>()->setQuadratic(0.000015f);
		// player->getComponent<Light>()->setInnerCutoff(radians(7.5f));
		// player->getComponent<Light>()->setOuterCutoff(radians(9.f));

		////////////////////////////////////////////

		// player->_addComponent<collider>()->layer = 1;
		// player->_addComponent<rigidBody>()->bounciness = 0.3;
		// player->_addComponent<rigidBody>()->gravity = false;

		////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////

		// player->transform->translate(vec3(0, 10, -35));

		_model turretm("res/models/ship1/maingun.obj");
		_model gunsm("res/models/ship1/3guns.obj");
		_model shipModel = _model("res/models/ship1/ship.obj");

		REG_ASSET(turretm);
		REG_ASSET(gunsm);
		REG_ASSET(shipModel);

		// game_object *player_prox = new game_object();
		game_object *player_prox = player;
		player_prox->transform->setPosition({0, 10, -35});
		game_object *boom = _instantiate();
		boom->transform->adopt(player_prox->transform);
		// auto b = boom->_addComponent<_boom>();

		auto pointer =_instantiate();
		pointer->transform->setPosition(player_prox->transform->getPosition());
		player_prox->transform->adopt(pointer->transform);
		pointer->transform->translate(vec3(0, 0, 5000));
		pointer->_addComponent<_renderer>()->set(modelShader, cubeModel);

		game_object *ship = _instantiate();
		ship->transform->name() = "ship";
		auto r_ = ship->_addComponent<_renderer>();
		r_->set(modelShader, shipModel);
		ship->_addComponent<_ship>()->rotationSpeed = glm::radians(20.f);
		ship->getComponent<_ship>()->maxForward = 50.f;
		ship->getComponent<_ship>()->maxReverse = 20.f;
		ship->getComponent<_ship>()->thrust = 8.f;
		auto ship_col = ship->_addComponent<collider>();
		ship_col->setMesh(&shipModel.mesh());
		ship_col->dim = vec3(4, 2, 20);
		ship_col->layer = 1;

		vector<vec2> MainGunPos_s = {vec2(1.2, 7.0),
									 vec2(1.7, 4.45),
									 vec2(1.7, -5.2),
									 vec2(1.2, -8.2),
									 vec2(-1.2, 5.85),
									 vec2(-1.7, 3.05),
									 vec2(-1.7, -4.25),
									 vec2(-1.2, -7.1)};
		for (auto &i : MainGunPos_s)
		{
			makeGun(ship->transform, vec3(0, i.x, i.y), pointer->transform, i.y > 0, i.x > 0, bomb_proto);
		}
		ship->_addComponent<gunManager>();

		ship->_addComponent<Light>();
		ship->getComponent<Light>()->setColor(vec3(100, 0, 0));
		ship->getComponent<Light>()->setConstant(1.f);
		ship->getComponent<Light>()->setlinear(0.01f);
		ship->getComponent<Light>()->setQuadratic(0.0032f);
		ship->getComponent<Light>()->setOuterCutoff(radians(5.f));
		ship->getComponent<Light>()->setInnerCutoff(radians(4.9f));

		game_object *engine = _instantiate();
		engine->_addComponent<particle_emitter>()->setPrototype(getNamedEmitterProto("engineTrail"));
		engine->_addComponent<particle_emitter>()->setPrototype(getNamedEmitterProto("engineFlame"));
		engine->transform->translate(vec3(0, 0, -10));
		ship->transform->adopt(engine->transform);
		engine = _instantiate(*engine);
		engine->transform->translate(vec3(-2.2, 0, 6));
		engine = _instantiate(*engine);
		engine->transform->translate(vec3(2.2 * 2, 0, 0));

		game_object *ship_container = _instantiate();
		ship_container->transform->adopt(ship->transform);
		ship_container->transform->adopt(boom->transform);
		player_prox->_addComponent<player_sc>();

		////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////
		// ship->transform->adopt(boom->transform);

		// ship->transform->setScale(vec3(10));

		game_object_proto_ *tree_go = new game_object_proto_();
		tree_go->name = "tree";
		_renderer *tree_rend = tree_go->addComponent<_renderer>();
		tree_rend->set(modelShader, treeModel);
		registerProto(tree_go);

		game_object_proto_ *ground = new game_object_proto_();
		ground->name = "ground";
		// ground->transform->scale(vec3(20));
		auto r = ground->addComponent<_renderer>();
		_model groundModel = _model();
		// groundModel.makeUnique();
		r->set(terrainShader, groundModel);
		ground->addComponent<terrain>()->scatter_obj = tree_go;
		registerProto(ground);

		// terrainWidth = 1024;
		terrainWidth = 1024;
		// terrainWidth = 32;
		// int terrainsDim = 0;
		int terrainsDim = 0;
		// int terrainsDim = 16;

		// tree_rend->setCullSizes(0.04f, INFINITY);
		// _renderer *tree_billboard = tree_go->_addComponent<_renderer>();
		// _texture tree_bill_tex;
		// tree_bill_tex.namedTexture("bill1");
		// tree_bill_tex.setType("texture_diffuse");
		// waitForRenderJob([&]() {
		// 	tree_bill_tex.t->gen(1024, 1024);
		// });
		// makeBillboard(tree, tree_bill_tex, tree_billboard);
		// tree_billboard->setCullSizes(0.0f, 0.05f);
		// tree_go->transform->rotate(vec3(1,0,0),radians(-90.f));
		auto grnd = game_object_prototype(ground);
		for (int i = -terrainsDim; i < terrainsDim + 1; i++)
		{
			for (int j = -terrainsDim; j < terrainsDim + 1; j++)
			{
				game_object *g = _instantiate(grnd);
				g->transform.name() = "terrain:" + to_string(i) + "," + to_string(j);
				terrain *t = g->getComponent<terrain>();
				// t->scatter_obj = tree_go;
				// t->r = g->getComponent<_renderer>();
				// g->_addComponent<_renderer>()->set(wireFrame, t->r->getModel());

				g->transform->setScale(vec3(20));
				g->transform->setPosition(vec3(20 * i * terrainWidth, -4000, 20 * j * terrainWidth));
				t->width = t->depth = terrainWidth + 1;
				t->offsetX =  i * terrainWidth - terrainWidth * 0.5f;
				t->offsetZ =  j * terrainWidth - terrainWidth * 0.5f;
				// t->genHeightMap(terrainWidth, terrainWidth, i * terrainWidth - terrainWidth * 0.5f, j * terrainWidth - terrainWidth * 0.5f);
				if (i == 0 && j == 0)
				{
					terr = t;
				}
			}
		}

		// player->getComponent<player_sc>()->t = t;
		ifstream config("config.txt");
		int n;
		int numshooters;
		config >> n;
		config >> numshooters;
		srand(100);
		game_object_prototype bmb(bomb_proto);
		game_object *CUBE = _instantiate(bmb);
		// CUBE->getComponent<physicsObject>()->init(vec3(100));

		////////////////////////////////////////////////

		//gameObjects.front()->_addComponent<mvpSolver>();

		// proto = CUBE;

		game_object *shooter = _instantiate();
		// shooter->transform->setRotation(lookAt(vec3(0),vec3(0,1,0),vec3(0,0,1)));
		shooter->transform->move(vec3(0, 100, 0));
		shooter->transform->scale(glm::vec3(3));
		shooter->_addComponent<_renderer>()->set(modelShader, cubeModel);
		// shooter->_addComponent<_renderer>()->set(modelShader, _model("res/models/ship1/ship.obj"));
		gun *g = shooter->_addComponent<gun>();
		g->ammo = bomb_proto;
		g->rof = 100;
		g->dispersion = 0.5;
		g->speed = 100;
		// g->ammo = bullets["bomb"].proto;
		shooter->_addComponent<autoShooter>();
		shooter->_addComponent<collider>()->setLayer(1);
		shooter->getComponent<collider>()->dim = vec3(1);
		// shooter->transform->setScale(vec3(6));
		game_object *go = _instantiate(*shooter);

		auto nanosuitMan = _instantiate(*CUBE);
		nanosuitMan->_addComponent<_renderer>();
		nanosuitMan->getComponent<_renderer>()->set(wireFrame2, nanoSuitModel);
		// cube_sc *it = nanosuitMan->getComponent<cube_sc>();
		// nanosuitMan->removeComponent<cube_sc>(it);
		nanosuitMan->transform->move(glm::vec3(-10.f));
		nanosuitMan->removeComponent<missile>();

		emitter_prototype_ ep2 = createNamedEmitter("emitter2");
		*ep2 = *getNamedEmitterProto("flame");
		ep2->billboard = 0;
		ep2->trail = 0;
		ep2->emission_rate = 5.0f;
		ep2->lifetime = 7.f;
		ep2->maxSpeed = 1;
		ep2.color(vec4(1, .4, 0, 0.5), vec4(1, .4, 0, 0.0));
		auto pe = nanosuitMan->_addComponent<particle_emitter>();
		pe->setPrototype(ep2);

		game_object *proto2 = _instantiate();
		proto2->transform->translate(vec3(50));
		proto2->_addComponent<spinner>();
		proto2->_addComponent<_renderer>();
		// proto2->_addComponent<rigidBody>()->gravity = false;
		proto2->_addComponent<collider>()->setLayer(1);
		// proto2->_addComponent<cube_sc>();
		proto2->getComponent<_renderer>()->set(modelShader, cubeModel);
		proto2->_addComponent<particle_emitter>();
		// proto2->removeComponent<cube_sc>();
		// proto2->removeComponent<particle_emitter>();
		proto2->getComponent<particle_emitter>()->setPrototype(ep2);
		proto2->transform->setScale(glm::vec3(10.f));
		proto2->transform->translate(glm::vec3(10.f) * 0.5f);

		// create big cubes
		for (int i = 0; i < 5; ++i) //20
		{
			proto2 = _instantiate(*proto2);
			proto2->transform.name() = "box " + to_string(i);
			proto2->transform->setScale(glm::vec3(pow(10.f, (float)(i + 1))));
			proto2->transform->scale(vec3(2, 1, 1));
			proto2->transform->translate(glm::vec3(pow(10.f, (float)(i + 1))) * 4.f);
			proto2->transform->rotate(randomSphere(), randf() * 1.5);
		}

		// create shooters
		for (int i = 0; i < 300; ++i)
		{
			go = _instantiate(*go);
			go->transform.name() = "shooter " + to_string(i);
			go->transform->translate(randomSphere() * 1000.f);
			vec3 pos = go->transform->getPosition();
			go->transform->setPosition(vec3(fmod(pos.x, 8000), fmod(pos.y, 300.f) + 400.f, fmod(pos.z, 8000)));
			go->transform->rotate(randomSphere(), randf() * 10.f);
			// if (fmod((float)i, (n / 100)) < 0.01)
			// cout << "\r" << (float)i / (float)n << "    " << flush;
		}
	}
	else
	{
		// load_game("game.lvl");
	}
	// 	_model cubeModel("res/models/cube/cube.obj");
	// while (!cubeModel.meta()->model->ready())
	// 		this_thread::yield();
	// boxPoints = cubeModel.meta()->model->meshes[0].vertices;
	// boxTris = cubeModel.meta()->model->meshes[0].indices;

	return 0;
}
