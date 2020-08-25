#include "game_engine.h"
// #include "initMain.h"
#include <fstream>
#include <iostream>
#include <string>

#include <iomanip>
#include <locale>
#include "bullet/src/BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h"

terrain* terr;
int numBoxes = 0;

audio gunSound;
game_object* physObj;
// int colCount = 0;


using namespace glm;

game_object *proto = nullptr;
game_object *ExplosionProto = nullptr;
emitter_prototype_ _expSmoke;
emitter_prototype_ _expFlame;



struct bullet {
	game_object_proto* proto;
	emitter_prototype_ primarybullet;
	// emitter_prototype_ secondarybullet;
	emitter_prototype_ primaryexplosion;
	// emitter_prototype_ secondaryexplosion;
};

map<string, bullet> bullets;


class missile : public component
{
public:
	// rigidBody *rb;
	vec3 vel;
	bullet b;
	// glm::vec3 rot;
	// particle_emitter* myEmitter;
	// glm::vec3 dir;
	// bool hit = false;
	// audiosource* sound;
	// double life;
	missile() {}
	void onStart()
	{
		// rot = randomSphere();
		// hit = false;
		numCubes.fetch_add(1);
		// sound = transform->gameObject->getComponent<audiosource>();
		// myEmitter = transform->gameObject->getComponent<particle_emitter>();
	}
	void setBullet(const bullet& _b){
		b = _b;
		// myEmitter->setPrototype(b.primarybullet);
		// myEmitters[1]->setPrototype(b.secondarybullet);
	}
	void update()
	{
			transform->move(vel * Time.deltaTime);
			// transform->rotate(rot, Time.deltaTime * glm::radians(100.f));
			vel += vec3(0, -9.81, 0) * Time.deltaTime;

		// if(hit){

		// 	numCubes.fetch_add(-1);
		// 	// colCount++;
		// 	// // if(length(normal) == 0)
		// 	// vec3 normal;
		// 	// 	normal = randomSphere();
		// 	// b.primaryexplosion.burst(transform->getPosition(),normal,transform->getScale(),10);
		// 	transform->gameObject->destroy();
		// }

	}
	void onCollision(game_object *go,vec3 point, vec3 normal)
	{
		// if(!hit && go->getComponent<missile>() == 0){
			// colCount++;
			if(length(normal) == 0)
				normal = randomSphere();
			b.primaryexplosion.burst(transform->getPosition(),normal,transform->getScale(),10);
			numCubes.fetch_add(-1);
			transform->gameObject->destroy();

			// b.primaryexplosion.burst(transform->getPosition(),normal,transform->getScale(),10);
			// sound->play(transform->getPosition());
			// getEmitterPrototypeByName("shockWave").burst(transform->getPosition(),normal,transform->getScale(),25);
			// getEmitterPrototypeByName("debris").burst(transform->getPosition(),normal,transform->getScale(),7);
			// hit = true;
		// }
	}
	//UPDATE(missile, update);
	COPY(missile);
};

class gun : public component
{
	float reload;
	float lastFire;
	vector<vec3> barrels = {vec3(0,-12,10)};
public:
	float rof;
	float speed;
	float dispersion;
	game_object_proto* ammo;
	float size = 1;
	void onStart(){
		lastFire = Time.time;
	}
	void setBarrels(vector<vec3> b){
		barrels = b;
	}
	bool isReloaded(){
		return Time.time - lastFire > 1.f / rof;
	}
	bool fire()
	{
		
		// reload += rof * Time.deltaTime;
		if(Time.time - lastFire > 1.f / rof){

			reload = glm::max(Time.deltaTime * rof,1.f);
			lastFire = Time.time;
			for (int i = 0; i < (int)reload; i++)
			{
				for(auto& j : barrels){
					game_object *go = new game_object(*ammo);
					go->transform->setScale(vec3(size));
					go->transform->setPosition(transform->getPosition() + vec3(toMat4(transform->getRotation()) * scale(transform->getScale()) * vec4(j,1)));
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
	COPY(gun);
};

float ship_accel;
float ship_vel;
class player_sc : public component
{
	bool cursorReleased = false;
	float speed = 10.f;
	// rigidBody *rb;
	bool flying = true;
	bool jumped = false; // do not fly and jump in same frame
	int framecount = 0;
	vec3 ownSpeed = vec3(0);
	bullet bomb;
	bullet laser;
	vector<gun*> guns;
	float rotationSpeed = 10.f;
	float rotX;
	float rotY;
	float fov;
	_camera* cam;
	
	gui::window* info;
	gui::text* fps;
	gui::text* missileCounter;
	gui::text* particleCounter;
	gui::text* shipAcceleration;
	gui::text* shipVelocity;
	gui::text* lockedfrustum;
	gui::text* colCounter;
	gui::window* reticule;
	gui::image* crosshair;
	_texture crosshairtex;
public:
	terrain *t;

	// void onCollision(game_object *collidee)
	// {
	// 	colliding = true;
	// }

	void onStart()
	{
		// rb = transform->gameObject->getComponent<rigidBody>();
		guns = transform->gameObject->getComponents<gun>();
		// bomb = bullets["bomb"];
		guns[0]->ammo = bullets["bomb"].proto;
		guns[0]->rof = 3'000 / 60;
		guns[0]->dispersion = 0.3f;
		guns[0]->speed = 200;
		// laser = bullets["laser"];
		guns[1]->ammo = bullets["laser"].proto;
		guns[1]->rof = 1000 / 60;
		guns[1]->dispersion = 0;
		guns[1]->speed = 30000;
		guns[1]->size = 20;
		guns[0]->setBarrels({vec3(0.f,-10.f,45.f)});

		info = new gui::window();
		fps = new gui::text();
		missileCounter = new gui::text();
		particleCounter = new gui::text();
		shipAcceleration = new gui::text();
		shipVelocity = new gui::text();
		lockedfrustum = new gui::text();
		colCounter = new gui::text();
		info->name = "game info";
		ImGuiWindowFlags flags = 0;
		flags |= ImGuiWindowFlags_NoTitleBar;
		flags |= ImGuiWindowFlags_NoMove;
		flags |= ImGuiWindowFlags_NoResize;
		// flags |= ImGuiWindowFlags_NoBackground;
		info->flags = flags;
		info->pos = ImVec2(20,20);
		info->size = ImVec2(200,150);
		info->children.push_back(fps);
		info->children.push_back(missileCounter);
		info->children.push_back(particleCounter);
		info->children.push_back(lockedfrustum);
		info->children.push_back(shipAcceleration);
		info->children.push_back(shipVelocity);
		info->adopt(colCounter);
		reticule = new gui::window();
		flags |= ImGuiWindowFlags_NoBackground;
		flags |= ImGuiWindowFlags_NoScrollbar;
		flags &= ~ImGuiWindowFlags_NoMove;
		reticule->flags = flags;
		reticule->name = "reticule";
		reticule->pos = ImVec2(0,0);
		crosshair = new gui::image();
		waitForRenderJob([&](){crosshairtex.load("res/images/crosshair.png");});
		crosshair->img = crosshairtex;
		reticule->adopt(crosshair);
		cam = transform->gameObject->getComponent<_camera>();
		fov = cam->fov;

	}
	void update()
	{

		// rb->gravity = false;
		transform->getParent()->rotate(inverse(transform->getParent()->getRotation()) * vec3(0,1,0), Input.Mouse.getX() * Time.unscaledDeltaTime * rotationSpeed * fov / 80 * -0.01f);
		transform->getParent()->rotate(vec3(1,0,0), Input.Mouse.getY() * Time.unscaledDeltaTime * rotationSpeed  * fov / 80  * -0.01f);

		// transform->translate(vec3(0,1,-4) * -Input.Mouse.getScroll());
		fov -= Input.Mouse.getScroll() * 5;
		fov = glm::clamp(fov, 5.f,80.f);
		transform->gameObject->getComponent<_camera>()->fov = fov;//Input.Mouse.getScroll();


		if (framecount++ > 1){

			fps->contents = "fps: " + to_string(1.f / Time.unscaledSmoothDeltaTime);
			missileCounter->contents = "missiles: " + FormatWithCommas(numCubes.load());
			particleCounter->contents = "particles: " + FormatWithCommas(getParticleCount());
			shipVelocity->contents = "speed: " + to_string(ship_vel);
			shipAcceleration->contents = "thrust: " + to_string(ship_accel);
			lockedfrustum->contents = "locked frustum: " + to_string(transform->gameObject->getComponent<_camera>()->lockFrustum);
			reticule->size = ImVec2(SCREEN_WIDTH,SCREEN_HEIGHT);
			crosshair->pos = ImVec2(SCREEN_WIDTH / 2 - 240,SCREEN_HEIGHT / 2 - 200);
			colCounter->contents = "collisions: " + to_string(colCount);
		}

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
		// if (Input.Mouse.getButton(GLFW_MOUSE_BUTTON_LEFT)){
		// 	for(int i = 0; i <= Time.deltaTime * 100; i++){
		// 		numBoxes++;
		// 		auto g = new game_object(*physObj);
		// 		vec3 r = randomSphere() * 2.f * randf() + transform->getPosition() + transform->forward() * 12.f;
		// 		physObj->getComponent<physicsObject>()->init(r.x,r.y,r.z, transform->forward() * 30.f + randomSphere()*10.f);
		// 	}
		// 	// guns[0]->fire();
		// }
		if (Input.Mouse.getButton(GLFW_MOUSE_BUTTON_RIGHT)){
			// for(int i = 0; i <= Time.deltaTime * 100; i++){
			// 	numBoxes++;
			// 	auto g = new game_object(*physObj);
			// 	vec3 r = randomSphere() * 2.f * randf() + transform->getPosition() + transform->forward() * 12.f;
			// 	physObj->getComponent<physicsObject>()->init(r.x,r.y,r.z, transform->forward() * 30.f + randomSphere()*10.f);
			// }
			guns[0]->fire();
		}

	}
	//UPDATE(player_sc, update);
	COPY(player_sc);
};

// class nbody : public component
// {
// 	//    glm::vec3 vel;
// 	rigidBody *rb;

// public:
// 	void onStart()
// 	{
// 		rb = transform->gameObject->getComponent<rigidBody>();
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
	//UPDATE(spinner,update);
	COPY(spinner);
};



const float _pi = radians(180.f);
class _turret : public component{
	Transform* target;
	Transform* guns;
	emitter_prototype_ muzzelFlash;
	// emitter_prototype_ muzzelSmoke;
	gun* barrels;
	audiosource* sound;
	float turret_angle;
	float guns_angle;
	bool canFire;

public:
	float turret_speed = radians(30.f);
	float gun_speed = radians(30.f);
	float t_angles[3];
	float g_angles[3][2];
	void setTarget(Transform* t){
		target = t;
	}
	float getRateOfFire(){
		return barrels->rof;
	}
	void onStart(){
		guns = transform->getChildren().front();
		barrels = guns->gameObject->getComponent<gun>();
		sound = transform->gameObject->getComponent<audiosource>();
		muzzelFlash = getEmitterPrototypeByName("muzzelFlash");
		// muzzelSmoke = getEmitterPrototypeByName("muzzelSmoke");
	}
	void update(){
		/////////////////////////////////////////////// turn turret
		vec3 targetPos = inverse(toMat3(transform->getRotation())) * (target->getPosition() - transform->getPosition());
		targetPos = normalize(vec3(targetPos.x,0,targetPos.z));
		float angle = acos(targetPos.z);
		float turn_angle = std::min(turret_speed * Time.deltaTime,angle) * (targetPos.x > 0 ? 1 : -1);
		angle *= (targetPos.x > 0 ? 1 : -1);

		if((turret_angle + angle > 0 && turret_angle + angle - _pi > 0) || (turret_angle + angle < 0 && turret_angle + angle + _pi < 0)){
			turn_angle = turret_speed * Time.deltaTime * (targetPos.x > 0 ? -1 : 1);
		}
		if(turret_angle + turn_angle > t_angles[2]){ // left max angle
			turn_angle = t_angles[2] - turret_angle;
		}else if(turret_angle + turn_angle < t_angles[0]){ // right max angle
			turn_angle = t_angles[0] - turret_angle;
		}

		canFire = abs(angle) < turret_speed * Time.deltaTime;
		turret_angle += turn_angle;
		float turret_turn_angle = turn_angle - angle;
		transform->rotate(vec3(0,1,0),angle);

		/////////////////////////////////////////////// turn guns
		targetPos = inverse(toMat3(guns->getRotation())) * (target->getPosition() - transform->getPosition());
		targetPos = normalize(vec3(0,targetPos.y,targetPos.z));
		angle = acos(targetPos.z);
		turn_angle = std::min(gun_speed * Time.deltaTime,angle) * (targetPos.y > 0 ? -1 : 1);
		angle *= (targetPos.y > 0 ? 1 : -1);

		int index = (turret_angle > 0 ? 2 : 0);
		float ratio = abs(turret_angle / t_angles[index]);
		float guns_angles[2] = {g_angles[index][0] * ratio + g_angles[1][0] * (1 - ratio),
		g_angles[index][1] * ratio + g_angles[1][1] * (1 - ratio)};

		// if((guns_angle + angle > 0 && guns_angle + angle - pi > 0) || (guns_angle + angle < 0 && guns_angle + angle + pi < 0)){
		// 	turn_angle = gun_speed * Time.deltaTime * (targetPos.x > 0 ? -1 : 1);
		// }
		if(guns_angle + turn_angle > guns_angles[1]){ // up max angle
			turn_angle = guns_angles[1] - guns_angle;
		}else if(guns_angle + turn_angle < guns_angles[0]){ // down max angle
			turn_angle = guns_angles[0] - guns_angle;
		}

		guns_angle += turn_angle;
		guns->rotate(vec3(1,0,0),turn_angle);

		transform->rotate(vec3(0,1,0),turret_turn_angle);

		canFire = canFire && abs(angle) < gun_speed * Time.deltaTime;
		// if(canFire && Input.Mouse.getButton(GLFW_MOUSE_BUTTON_LEFT)){
		// 	if(barrels->fire()){
		// 		// cout << "fire" << endl;
		// 		muzzelFlash.burst(guns->forward() * guns->getScale() * 5.3f + guns->getPosition(),guns->forward(),20);
		// 		getEmitterPrototypeByName("shockWave").burst(transform->getPosition(),guns->forward(),vec3(0.2),60);

		// 		// muzzelSmoke.burst(guns->forward() * guns->getScale() * 5.3f + guns->getPosition(),guns->forward(),17);
		// 	}
		// }
	}
	bool onTarget(){
		return canFire;
	}
	bool reloaded(){
		return barrels->isReloaded();
	}
	bool fire(){
		if(canFire){
			if(barrels->fire()){
				// cout << "fire" << endl;
				muzzelFlash.burst(guns->forward() * guns->getScale() * 5.3f + guns->getPosition(),guns->forward(),20);
				getEmitterPrototypeByName("shockWave").burst(transform->getPosition(),guns->forward(),vec3(0.2),60);
				sound->play();
				// muzzelSmoke.burst(guns->forward() * guns->getScale() * 5.3f + guns->getPosition(),guns->forward(),17);
				return true;
			}
		}
		return false;
	}
	//UPDATE(_turret,update);
	COPY(_turret);
};

class gunManager : public component{

	vector<_turret*> turrets;
	int curr = 0;
	double t;
	double t_;
public:
	void update(){
		if(Input.Mouse.getButton(GLFW_MOUSE_BUTTON_LEFT)){
			// for(auto& i : turrets){
				if(Time.time > t){
					for(auto& i : turrets){
						if(i->reloaded() && i->onTarget()){
							i->fire();
							t = Time.time + t_;
							curr = (curr + 1) % turrets.size();
							break;
						}
					}
				}
			// }
		}

	}
public:
	void onStart(){
		for(auto& i : transform->getChildren()){
			auto tur = i->gameObject->getComponent<_turret>();
			if(tur != 0){
				turrets.push_back(tur);
			}
		}
		t_ = (1.f / turrets[0]->getRateOfFire()) / turrets.size();
	}
public:
	//UPDATE(gunManager,update);
	COPY(gunManager);
};

class autoShooter : public component
{
public:
	bool shouldFire;
	gun* g;
	void onStart(){
		g = transform->gameObject->getComponent<gun>();
	}
	void update()
	{
		if(Input.getKey(GLFW_KEY_H))
			shouldFire = false;
		else if(Input.getKey(GLFW_KEY_J))
			shouldFire = true;
		if(shouldFire)
			g->fire();
	}
	//UPDATE(autoShooter, update);
	COPY(autoShooter);
};

void makeGun(Transform* ship,vec3 pos,Transform* target, bool forward, bool upright){
	_shader modelShader("res/shaders/model.vert", "res/shaders/model.frag");
	_model turretm("res/models/ship1/maingun.obj");
	_model gunsm("res/models/ship1/3guns.obj");
	
	game_object* turret = new game_object();
	turret->addComponent<audiosource>()->set(gunSound);
	auto r = turret->addComponent<_renderer>();
	r->set(modelShader,turretm);
	game_object* guns = new game_object();
	r = guns->addComponent<_renderer>();
	r->set(modelShader, gunsm);
	turret->transform->Adopt(guns->transform);
	guns->transform->translate(vec3(0,-0.1,0.6));
	vector<vec3> barrels = {vec3(-.56,0,2.3),vec3(0,0,2.3),vec3(0.56,0,2.3)};
	auto g = guns->addComponent<gun>();
	g->setBarrels(barrels);
	g->ammo = bullets["bomb"].proto;
	g->rof = 1.f / 4.f;
	g->dispersion = 0.01f;
	g->speed = 500;

	ship->Adopt(turret->transform);
	turret->transform->translate(pos);

	if(!upright){
		turret->transform->rotate(vec3(0,0,1),radians(180.f));
	}
	if(!forward){
		turret->transform->rotate(vec3(0,1,0),radians(180.f));
	}
	auto t = turret->addComponent<_turret>();
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

class _ship : public component{
	vec3 vel;
public:
	float accel;
	float thrust;
	float maxReverse;
	float maxForward;
	float rotationSpeed;
	void onStart(){
		accel = 0;
		thrust = 20;
		maxReverse = 50;
		maxForward = 100;
		rotationSpeed = radians(10.f);
	}
	void update(){
		transform->rotate(glm::vec3(0, 1, 0), (Input.getKey(GLFW_KEY_A) - Input.getKey(GLFW_KEY_D)) * Time.deltaTime * rotationSpeed);
		transform->rotate(glm::vec3(1, 0, 0), (Input.getKey(GLFW_KEY_W) - Input.getKey(GLFW_KEY_S)) * Time.deltaTime * rotationSpeed);
		transform->rotate(glm::vec3(0, 0, 1), (Input.getKey(GLFW_KEY_SPACE) - Input.getKey(GLFW_KEY_LEFT_SHIFT)) * Time.deltaTime * rotationSpeed);

		vel -= vel * 0.4f * Time.deltaTime;
		vel += transform->forward() * accel * 0.4f * Time.deltaTime;
		ship_vel = length(vel);
		ship_accel = accel;
		// if(length(vel) > maxForward){
		// 	vel = normalize(vel) * maxForward;
		// }
		accel = glm::clamp(accel + (Input.getKey(GLFW_KEY_R) - Input.getKey(GLFW_KEY_F)) * thrust * Time.deltaTime,-maxReverse,maxForward);
		// cout << " accel: " << accel << endl;
		transform->getParent()->move(vel * Time.deltaTime,true);
		// vec3 inputVel = ((float)(Input.getKey(GLFW_KEY_A) - Input.getKey(GLFW_KEY_D)) * transform->right()
		//  + (float)(Input.getKey(GLFW_KEY_SPACE) - Input.getKey(GLFW_KEY_LEFT_SHIFT)) * transform->up()
		//  + (float)(Input.getKey(GLFW_KEY_W) - Input.getKey(GLFW_KEY_S)) * transform->forward());
	}
	//UPDATE(_ship,update);
	COPY(_ship);
};

class _boom : public component{
public:
	Transform* t;
	void update(){
		transform->setPosition(t->getPosition());
	}
	//UPDATE(_boom,update);
	COPY(_boom);
};


class player_sc2 : public component {
    float speed = 3.f;
    bool cursorReleased = false;
	float fov = 80;
	gui::window* info;
	gui::text* fps;
	vector<gun*> guns;
public:
    void onStart(){
		info = new gui::window();
		fps = new gui::text();
		info->adopt(fps);
		info->name = "game info";
		// ImGuiWindowFlags flags = 0;
		// flags |= ImGuiWindowFlags_NoTitleBar;
		// flags |= ImGuiWindowFlags_NoMove;
		// flags |= ImGuiWindowFlags_NoResize;
		// info->flags = flags;
		info->pos = ImVec2(20,20);
		info->size = ImVec2(200,150);

		guns = transform->gameObject->getComponents<gun>();
		// bomb = bullets["bomb"];
		guns[0]->ammo = bullets["bomb"].proto;
		guns[0]->rof = 3'000 / 60;
		guns[0]->dispersion = 0.3f;
		guns[0]->speed = 200;
		guns[0]->setBarrels({vec3(-1.1,0.4,0.5)});
	}
    void update(){
        fps->contents = "fps: " + to_string(1.f / Time.unscaledSmoothDeltaTime);

        transform->translate(glm::vec3(1, 0, 0) * (float)(Input.getKey(GLFW_KEY_A) - Input.getKey(GLFW_KEY_D)) * Time.deltaTime * speed);
		transform->translate(glm::vec3(0, 0, 1) * (float)(Input.getKey(GLFW_KEY_W) - Input.getKey(GLFW_KEY_S)) * Time.deltaTime * speed);
		transform->translate(glm::vec3(0, 1, 0) * (float)(Input.getKey(GLFW_KEY_SPACE) - Input.getKey(GLFW_KEY_LEFT_SHIFT)) * Time.deltaTime * speed);
        transform->rotate(glm::vec3(0, 0, 1), (float)(Input.getKey(GLFW_KEY_Q) - Input.getKey(GLFW_KEY_E)) * -Time.deltaTime);
		transform->rotate(vec3(0,1,0), Input.Mouse.getX() * Time.unscaledDeltaTime  * fov / 80 * -0.4f);
        transform->rotate(vec3(1,0,0), Input.Mouse.getY() * Time.unscaledDeltaTime  * fov / 80 * -0.4f);

		fov -= Input.Mouse.getScroll() * 5;
		fov = glm::clamp(fov, 5.f,80.f);
		transform->gameObject->getComponent<_camera>()->fov = fov;//Input.Mouse.getScroll();

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
		if(Input.getKeyDown(GLFW_KEY_R)){
			speed *= 2;
		}
		if(Input.getKeyDown(GLFW_KEY_F)){
			speed /= 2;
		}
		if (Input.Mouse.getButton(GLFW_MOUSE_BUTTON_LEFT)){
			guns[0]->fire();
		}
    }
    COPY(player_sc2);
    
};

class player_sc3 : public component {
public:
	float speed = 3.f;
    bool cursorReleased = false;
	float fov = 80;
	gui::window* info;
	gui::text* fps;
	vector<gun*> guns;

	void onStart(){
		info = new gui::window();
		fps = new gui::text();
		info->adopt(fps);
		info->name = "game info";
		// ImGuiWindowFlags flags = 0;
		// flags |= ImGuiWindowFlags_NoTitleBar;
		// flags |= ImGuiWindowFlags_NoMove;
		// flags |= ImGuiWindowFlags_NoResize;
		// info->flags = flags;
		info->pos = ImVec2(20,20);
		info->size = ImVec2(200,150);

		guns = transform->gameObject->getComponents<gun>();
		// bomb = bullets["bomb"];
		guns[0]->ammo = bullets["bomb"].proto;
		guns[0]->rof = 3'000 / 60;
		guns[0]->dispersion = 0.3f;
		guns[0]->speed = 200;
		guns[0]->setBarrels({vec3(-1.1,0.4,0.5)});
	}
	void update(){
		fps->contents = "fps: " + to_string(1.f / Time.unscaledSmoothDeltaTime);

		transform->translate(glm::vec3(1, 0, 0) * (float)(Input.getKey(GLFW_KEY_A) - Input.getKey(GLFW_KEY_D)) * Time.deltaTime * speed);
		transform->translate(glm::vec3(0, 0, 1) * (float)(Input.getKey(GLFW_KEY_W) - Input.getKey(GLFW_KEY_S)) * Time.deltaTime * speed);
		transform->rotate(vec3(0,1,0), Input.Mouse.getX() * Time.unscaledDeltaTime  * fov / 80 * -0.4f);
        transform->rotate(vec3(1,0,0), Input.Mouse.getY() * Time.unscaledDeltaTime  * fov / 80 * -0.4f);
		vec3 pos = transform->getPosition();
		pos.y = getTerrain(pos.x,pos.z)->getHeight(pos.x,pos.z).height + 1.8;
		transform->setPosition(pos);
		// transform->setRotation(quatLookAtLH(transform->getRotation() * vec3(0,0,1),vec3(0,1,0)));
		transform->lookat(transform->forward(),vec3(0,1,0));

		fov -= Input.Mouse.getScroll() * 5;
		fov = glm::clamp(fov, 5.f,80.f);
		transform->gameObject->getComponent<_camera>()->fov = fov;//Input.Mouse.getScroll();

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
		if(Input.getKeyDown(GLFW_KEY_R)){
			speed *= 2;
		}
		if(Input.getKeyDown(GLFW_KEY_F)){
			speed /= 2;
		}
		if (Input.Mouse.getButton(GLFW_MOUSE_BUTTON_LEFT)){
			guns[0]->fire();
		}

	}
    COPY(player_sc3);
};


int main(int argc, char **argv)
{
	if (argc > 1)
		maxGameDuration = (float)stoi(argv[1]);

	hideMouse = false;
	
	cout << sizeof(tbb::spin_mutex) << " : " << sizeof(tbb::mutex) << endl;
	
	::init();
	gunSound = audio("res/audio/explosion1.wav");
	
	_shader modelShader("res/shaders/model.vert", "res/shaders/model.frag");
	_shader terrainShader("res/shaders/model.vert", "res/shaders/terrain.frag");
	_model cubeModel("res/models/cube/cube.obj");
	_model nanoSuitModel("res/models/nanosuit/nanosuit.obj");
	_model terrainModel("res/models/terrain/terrain.obj");
	_model tree("res/models/Spruce_obj/Spruce.obj");

	collisionGraph[0] = {1};
	collisionGraph[1] = {0,1};

	colorArray ca;
	ca.addKey(vec4(1),0.03)
	.addKey(vec4(1,1,0.5f,1),0.05)
	.addKey(vec4(1,0.8f,0.5f,0.9f),0.07)
	// .addKey(vec4(1,0.6f,0.5f,0.9f),0.09)
	.addKey(vec4(0.65,0.65,0.65,0.3f),0.09)
	.addKey(vec4(0.65,0.65,0.65,0.0f),8.f / 9.f);

	colorArray ca3;
	ca3.addKey(vec4(1),0.1)
	.addKey(vec4(1,1,0.9f,1),0.15)
	.addKey(vec4(1,0.8f,0.5f,0.9f),0.2)
	.addKey(vec4(1,0.5f,0.5f,0.9f),0.25)
	.addKey(vec4(0.75,0.65,0.54,0.6f),0.30)
	.addKey(vec4(0.75,0.65,0.54,0.0f),1.f);

	colorArray ca2;
	ca2.addKey(vec4(1),0.1)
	.addKey(vec4(1,1,0.9f,1),0.15)
	.addKey(vec4(1,0.8f,0.5f,0.9f),0.2)
	.addKey(vec4(1,0.5f,0.5f,0.9f),0.25)
	.addKey(vec4(0.5,0.5f,0.5f,0.6f),0.3)
	.addKey(vec4(0.5,0.5f,0.5f,0.0f),1);

	floatArray fa;
	fa.addKey(0.f,0.f).addKey(0.4f,0.02f).addKey(2.0,1.0);

	emitter_prototype_ flameEmitterProto = createNamedEmitter("flame");
	flameEmitterProto->dispersion = 3.14159f;
	flameEmitterProto->emission_rate = 2.f;
	flameEmitterProto->lifetime = 3.f;
	flameEmitterProto->lifetime2 = 3.f;
	flameEmitterProto->size(1.f);
	// flameEmitterProto->color(vec4(1, 1, 0.1f, 1.f));
	flameEmitterProto->maxSpeed = 1.f;
	flameEmitterProto->scale = vec3(1.f);
	flameEmitterProto->billboard = 1;
	flameEmitterProto->trail = 1;
	ca.setColorArray(flameEmitterProto->colorLife);

	emitter_prototype_ _muzzelFlash = createNamedEmitter("muzzelFlash");
	_muzzelFlash->dispersion = 0.5f;
	_muzzelFlash->emission_rate = 1.f;
	_muzzelFlash->lifetime = 4.f;
	_muzzelFlash->lifetime2 = 2.f;
	// _muzzelFlash->color(vec4(1, 1, 0.2f, 0.8f));
	_muzzelFlash->maxSpeed = (10.f);
	_muzzelFlash->size(0.5f,1.f);
	_muzzelFlash->scale = vec3(7.f);
	_muzzelFlash->trail = 0;
	ca.setColorArray(_muzzelFlash->colorLife);

	emitter_prototype_ expFlame = createNamedEmitter("expflame");
	expFlame->dispersion = 3.14159f / 2.f;
	expFlame->emission_rate = 50.f;
	expFlame->lifetime = 5.f;
	expFlame->lifetime2 = 2.f;
	expFlame->maxSpeed = 30.f;
	expFlame->scale = vec3(30.f);
	expFlame->size(0.5f,1.6f);
	// fa.setFloatArray(expFlame->sizeLife);
	expFlame->trail = 0;
	_expFlame = expFlame;
	ca2.setColorArray(expFlame->colorLife);

	emitter_prototype_ shockWave = createNamedEmitter("shockWave");
	shockWave->dispersion = 3.14159f / 2.f;
	// shockWave->emission_rate = 50.f;
	shockWave->lifetime = 0.3f;
	shockWave->lifetime2 = 0.1f;
	shockWave->maxSpeed = 300.f;
	shockWave->minSpeed = 250.f;
	shockWave->scale = vec3(40.f);
	shockWave->size(0.5f,1.f);
	shockWave->trail = 0;
	shockWave->color(vec4(1,1,1,0.6),vec4(1,1,1,0));

	emitter_prototype_ debris = createNamedEmitter("debris");
	debris->dispersion = 3.14159f / 2.f;
	// debris->emission_rate = 50.f;
	debris->lifetime = 5.f;
	debris->lifetime2 = 3.f;
	debris->maxSpeed = 15.f;
	debris->minSpeed = 10.f;
	debris->radius = 10.f;
	debris->scale = vec3(5,60,0)*2.f;
	debris->size(0.5f,1.f);
	debris->velAlign = 1;
	// debris->color(vec4(0.82,0.7,0.54,0.6),vec4(0.82,0.7,0.54,0));
	ca3.setColorArray(debris->colorLife);
	
	bullet laser;
	laser.primarybullet = createNamedEmitter("laserbeam");
	laser.primarybullet->dispersion = 3.14159f;
	laser.primarybullet->color(vec4(.8,.8,1,1),vec4(.6,.6,1,0.0));
	laser.primarybullet->lifetime = 0.3f;
	laser.primarybullet->lifetime2 = 0.3f;
	laser.primarybullet->emission_rate = 20.f;
	laser.primarybullet->trail = 1;
	laser.primarybullet->scale = vec3(20.f);
	laser.primarybullet->size(1.f);
	laser.primarybullet->maxSpeed = 1.f;
	laser.primaryexplosion = getEmitterPrototypeByName("expflame");

	emitter_prototype_ engineTrail = createNamedEmitter("engineTrail");
	*engineTrail = *flameEmitterProto;
	engineTrail->dispersion = 0.0f;
	engineTrail->emission_rate = 2.f;
	engineTrail->lifetime = 7.f;
	engineTrail->lifetime2 = 7.f;
	engineTrail->color(vec4(0.6f, 0.7f, 1.f, 0.6f),vec4(0.05f, 0.1f, 1.f, 0.0f));
	engineTrail->size(1.f);
	engineTrail->maxSpeed = (0.f);
	engineTrail->scale = vec3(1);
	engineTrail->trail = 1;

	emitter_prototype_ engineFlame = createNamedEmitter("engineFlame");
	engineFlame->dispersion = 0.5f;
	engineFlame->emission_rate = 15.f;
	engineFlame->lifetime = 4.f;
	engineFlame->lifetime2 = 3.f;
	engineFlame->color(vec4(0.2f, 0.5f, 0.9f, 0.2f),vec4(0.05f, 0.1f, 0.9f, 0.2f));
	engineFlame->maxSpeed = (-3.f);
	engineFlame->size(1.f,0.f);
	engineFlame->scale = vec3(2.f);
	engineFlame->trail = 0;

	bullet bomb;
	bomb.primarybullet = flameEmitterProto;
	bomb.primaryexplosion = expFlame;

	game_object_proto* bomb_proto = new game_object_proto();
	bomb_proto->addComponent<_renderer>()->set_proto(modelShader, cubeModel);
	bomb_proto->addComponent<collider>()->layer = 0;
	bomb_proto->getComponent<collider>()->dim = vec3(0.4f);
	// bomb_proto->addComponent<physicsObject>();
	// bomb_proto->addComponent<audiosource>()->set(gunSound);
	bomb_proto->addComponent<particle_emitter>();
	bomb.proto = bomb_proto;
	bomb_proto->addComponent<missile>()->setBullet(bomb);
	bullets["bomb"] = bomb;

	game_object_proto* laser_proto = new game_object_proto();
	laser_proto->addComponent<collider>()->layer = 0;
	laser_proto->addComponent<particle_emitter>();
	laser.proto = laser_proto;
	laser_proto->addComponent<missile>()->setBullet(laser);
	bullets["laser"] = laser;

	//////////////////////////////////////////////////////////

    game_object* light = new game_object();
    light->transform->setPosition(glm::vec3(30000));
    light->addComponent<Light>()->setColor(glm::vec3(24000));
    light->getComponent<Light>()->setConstant(1.f);
    light->getComponent<Light>()->setlinear(0.000014f);
    light->getComponent<Light>()->setQuadratic(0.000007f);

	// physObj = new game_object();
	// physObj->addComponent<_renderer>()->set(modelShader, cubeModel);
	// physObj->addComponent<physicsObject>()->init(vec3(0));
	// numBoxes += 61;
	// for(int i = 0; i < 60; i++){
	// 	auto g = new game_object(*physObj);
	// 	vec3 r = randomSphere() * 500.f * randf() + vec3(0,500,0);
	// 	g->transform->setPosition(r);
	// 	g->getComponent<physicsObject>()->init(vec3(0));
	// }

	game_object* player = new game_object();
	auto playerCam = player->addComponent<_camera>();
	playerCam->fov = 80;
	playerCam->farPlane = 1e32f;
	player->addComponent<gun>();
	player->addComponent<gun>();
	// player->addComponent<collider>()->layer = 1;
	// player->addComponent<rigidBody>()->bounciness = 0.3;
	// player->addComponent<rigidBody>()->gravity = false;
	player->addComponent<player_sc2>();
	player->transform->translate(vec3(0, 10, -35));


	////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////
	// game_object* boom = new game_object();
	// boom->transform->Adopt(player->transform);
	// // auto b = boom->addComponent<_boom>();

	// auto pointer = new game_object();
	// pointer->transform->setPosition(player->transform->getPosition());
	// player->transform->Adopt(pointer->transform);
	// pointer->transform->translate(vec3(0,0,5000));
	// pointer->addComponent<_renderer>()->set(modelShader, cubeModel);

	// game_object* ship = new game_object();
	// auto r_ = ship->addComponent<_renderer>();
	// _model shipModel = _model("res/models/ship1/ship.obj");
	// r_->set(modelShader,shipModel);
	// ship->addComponent<_ship>()->rotationSpeed = glm::radians(20.f);
	// auto ship_col = ship->addComponent<collider>();
	// ship_col->dim = vec3(2,1,18);
	// ship_col->layer = 1;

	// vector<vec2> MainGunPos_s = {vec2(1.2,7.0),
	// vec2(1.7,4.45),
	// vec2(1.7,-5.2),
	// vec2(1.2,-8.2),
	// vec2(-1.2,5.85),
	// vec2(-1.7,3.05),
	// vec2(-1.7,-4.25),
	// vec2(-1.2,-7.1)};
	// for(auto& i : MainGunPos_s){
	// 	makeGun(ship->transform,vec3(0,i.x,i.y),pointer->transform,i.y > 0,i.x > 0);
	// }
	// ship->addComponent<gunManager>();

	// ship->addComponent<Light>();
	// ship->getComponent<Light>()->setColor(vec3(100,0,0));
    // ship->getComponent<Light>()->setConstant(1.f);
    // ship->getComponent<Light>()->setlinear(0.01f);
    // ship->getComponent<Light>()->setQuadratic(0.0032f);
	// ship->getComponent<Light>()->setOuterCutoff(radians(5.f));
	// ship->getComponent<Light>()->setInnerCutoff(radians(4.9f));

	// game_object* engine = new game_object();
	// engine->addComponent<particle_emitter>()->setPrototype(getEmitterPrototypeByName("engineTrail"));
	// engine->addComponent<particle_emitter>()->setPrototype(getEmitterPrototypeByName("engineFlame"));
	// engine->transform->translate(vec3(0,0,-10));
	// ship->transform->Adopt(engine->transform);
	// engine = new game_object(*engine);
	// engine->transform->translate(vec3(-2.2,0,6));
	// engine = new game_object(*engine);
	// engine->transform->translate(vec3(2.2 * 2,0,0));
	

	// game_object* ship_container = new game_object();
	// ship_container->transform->Adopt(ship->transform);
	// ship_container->transform->Adopt(boom->transform);
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
	// ship->transform->Adopt(boom->transform);

	// ship->transform->setScale(vec3(10));
	seedRand(vec3(123456789,345678901,567890123));
    genNoise(513,513,4);
	game_object_proto *ground = new game_object_proto();
	// ground->transform->scale(vec3(20));
	auto r = ground->addComponent<_renderer>();
	_model groundModel = _model();
	groundModel.makeUnique();
	r->set(terrainShader, groundModel);
	ground->addComponent<terrain>();
	// t->r = r;
	// terr = t;
	// t->width = t->depth = 1024;
	// terrainWidth = 1024;
	// terrainWidth = 32;
	terrainWidth = 64;
    // t->genHeightMap(terrainWidth,terrainWidth,0,0);
	// ground->transform->translate(glm::vec3(0,-4500,0));

    // ground = new game_object(*ground);
    // terrain* t2 = ground->getComponent<terrain>();
    // t2->r = ground->getComponent<_renderer>();
    // t2->genHeightMap(terrainWidth,terrainWidth,1 * terrainWidth,1*terrainWidth);
    // ground->transform->setPosition(vec3(20 * 1 * terrainWidth,-4500,20 * 1 * terrainWidth));

	game_object_proto* tree_go = new game_object_proto();
	tree_go->addComponent<_renderer>()->set(modelShader,tree);
	// tree_go->transform->rotate(vec3(1,0,0),radians(-90.f));
	// int terrainsDim = 0;
	// int terrainsDim = 16;
	int terrainsDim = 8;
    for(int i = -terrainsDim; i < terrainsDim + 1; i++){
        for (int j = -terrainsDim; j < terrainsDim + 1; j++)
        {
            game_object* g = new game_object(*ground);
	        terrain* t = g->getComponent<terrain>();
			t->scatter_obj = tree_go;
            t->r = g->getComponent<_renderer>();
            g->transform->setScale(vec3(20));
            g->transform->setPosition(vec3(20 * i * terrainWidth,-4500,20 * j * terrainWidth));
            t->genHeightMap(terrainWidth,terrainWidth,i * terrainWidth,j*terrainWidth);
            if(i ==0 && j == 0){
                terr = t;
            }
        }
    }

	// game_object* tree_go = new game_object();
	// tree_go->addComponent<_renderer>()->set(modelShader,tree);
	// tree_go->transform->rotate(vec3(1,0,0),radians(-90.f));
	// for(int i = -100; i < 100; i++){
	// 	for(int j = -100; j < 100; j++){
	// 		float x = (i + randf()) * 8.f;
	// 		float z = (j + randf()) * 8.f;
	// 		terrain* t = getTerrain(x,z);
	// 		if(t != 0){7

	// 		terrainHit h = t->getHeight(x, z);
	// 		if(dot(h.normal, vec3(0,1,0)) > 0.85){
	// 			tree_go = new game_object(*tree_go);
	// 			tree_go->transform->setPosition(vec3(x,h.height,z));
	// 		}
	// 		}
	// 	}	
	// }

	// float minH = 10000;
	// float maxH = -10000;
	// float *heightData = new float[terrainWidth*terrainWidth];
	// // setRadial(heightData, 4, PHY_FLOAT);
	// int x = 0;
	// for(int i = 0; i < t->heightMap.size(); i++){
	// 	for(int j = 0; j < t->heightMap[i].size(); j++){
	// 		heightData[j * terrainWidth + i] = t->heightMap[i][j];
	// 		if(minH > t->heightMap[i][j])
	// 			minH = t->heightMap[i][j];
	// 		else if(maxH < t->heightMap[i][j])
	// 			maxH = t->heightMap[i][j];
	// 	}
	// }

	// //similar to createSphere
	// btTransform ter;
	// ter.setIdentity();
	// ter.setOrigin(btVector3(0,0,0));
	// // btStaticPlaneShape* plane=new btStaticPlaneShape(btVector3(0,1,0),0);
	// ter.setOrigin(btVector3(0,maxH * ground->transform->getScale().y * ground->transform->getScale().y / 2 + ground->transform->getPosition().y,0));
	// btHeightfieldTerrainShape* heightField = new btHeightfieldTerrainShape(terrainWidth,terrainWidth, heightData,btScalar(1.0),btScalar(0.0),btScalar(maxH * ground->transform->getScale().y),1,PHY_FLOAT,false);
	// btVector3 localScaling(ground->transform->getScale().x,ground->transform->getScale().y,ground->transform->getScale().z);
	// heightField->setLocalScaling(localScaling);
	// btMotionState* motion=new btDefaultMotionState(ter);
	// // btVector3 localInertia(0,0,0);
	// btRigidBody::btRigidBodyConstructionInfo info(0.0,motion,heightField);
	// // info.m_restitution = 0.02;
	// btRigidBody* body=new btRigidBody(info);
	// body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
	// body->setUserPointer(ground);
	// pm->addBody(body);

	// player->getComponent<player_sc>()->t = t;
	ifstream config("config.txt");
	int n;
	int numshooters;
	config >> n;
	config >> numshooters;
	srand(100);

	
	game_object *CUBE = new game_object(*bomb_proto);
	// CUBE->getComponent<physicsObject>()->init(vec3(100));

	////////////////////////////////////////////////

	//gameObjects.front()->addComponent<mvpSolver>();

	proto = CUBE;

	game_object *shooter = new game_object();
	// shooter->transform->setRotation(lookAt(vec3(0),vec3(0,1,0),vec3(0,0,1)));
	shooter->transform->move(vec3(0, 100, 0));
	shooter->addComponent<_renderer>()->set(modelShader, cubeModel);
	// shooter->addComponent<_renderer>()->set(modelShader, _model("res/models/ship1/ship.obj"));
	gun* g = shooter->addComponent<gun>();
	g->rof = 100;
	g->dispersion = 0.5;
	g->speed = 100;
	g->ammo = bullets["bomb"].proto;
	shooter->addComponent<autoShooter>();
	shooter->addComponent<collider>()->layer = 1;
	shooter->getComponent<collider>()->dim = vec3(2,1,18);
	// shooter->transform->setScale(vec3(6));
	game_object *go = new game_object(*shooter);

	auto nanosuitMan = new game_object(*CUBE);
	nanosuitMan->addComponent<_renderer>();
	nanosuitMan->getComponent<_renderer>()->set(modelShader, nanoSuitModel);
	// cube_sc *it = nanosuitMan->getComponent<cube_sc>();
	// nanosuitMan->removeComponent<cube_sc>(it);
	nanosuitMan->transform->move(glm::vec3(-10.f));
    nanosuitMan->removeComponent<missile>();

	emitter_prototype_ ep2 = createNamedEmitter("emitter2");
	*ep2 = *flameEmitterProto;
	ep2->billboard = 0;
	ep2->trail = 0;
	ep2->emission_rate = 5.0f;
	ep2->lifetime = 7.f;
	ep2->maxSpeed = 1;
	ep2->color(vec4(1, .4, 0, 0.5));
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
	for (int i = 0; i < 20; ++i)//20
	{
		proto2 = new game_object(*proto2);
		proto2->transform->setScale(glm::vec3(pow(10.f, (float)(i + 1))));
		proto2->transform->scale(vec3(2,1,1));
		proto2->transform->translate(glm::vec3(pow(10.f, (float)(i + 1))) * 4.f);
		proto2->transform->rotate(randomSphere(),randf() * 1.5);
	}

	// create shooters
	for (int i = 0; i < numshooters; ++i)
	{
		go = new game_object(*go);
		go->transform->translate(randomSphere() * 1000.f);
		vec3 pos = go->transform->getPosition();
		go->transform->setPosition(vec3(fmod(pos.x, 8000), fmod(pos.y, 300.f) + 100.f, fmod(pos.z, 8000)));
		go->transform->rotate(randomSphere(), randf() * 10.f);
		// if (fmod((float)i, (n / 100)) < 0.01)
		// cout << "\r" << (float)i / (float)n << "    " << flush;
	}

	// // // create blob of bombs
	// go = new game_object(*CUBE);
	// for (int i = 0; i < n; i++)
	// {
	// 	go = new game_object(*go);
	// 	go->transform->translate(randomSphere() * 3.f);
	// 	if (fmod((float)i, (n / 100)) < 0.01)
	// 		cout << "\r" << (float)i / (float)n << "    " << flush;
	// 	go->getComponent<missile>()->vel = randomSphere() * randf() * 100.f;
	// }

	run();

	cout << endl << "missiles: " + FormatWithCommas(numCubes.load());
	
	// delete[] heightData;

	
	return 0;
}
