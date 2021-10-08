#include "../game_engine.h"
#include <array>
class comp : public component
{
public:
    // _renderer *r;
    static transform2 orbiter;
    int updateCount = 0;
    glm::vec3 vel;
    void onStart()
    {
        // updateCount = numeric_limits<float>::max();
        // updateCount = Time.time + randf() * 50 + 50;
        transform->setRotation(glm::quatLookAtLH(randomSphere(), randomSphere()));
        updateCount = 100; //10 + (transform.id % 55);
    }
    void update()
    {
        float r2 = glm::length2(transform.getPosition());
        glm::vec3 dir = glm::normalize(transform.getPosition() - comp::orbiter->getPosition());
        vel += Time.deltaTime * glm::clamp(500.f / r2, 10.f, 10000.f) * -dir;
        // transform.rotate(vel, glm::radians(50.f) * Time.deltaTime);
        transform.move(vel * Time.deltaTime);
        // if (updateCount < Time.time)
        // {
        //     transform->gameObject()->destroy();
        // }
        // if (--updateCount <= 0)
        // {
        //     transform->gameObject()->destroy();
        // }
    }
    void onCollision(game_object *go, glm::vec3 point, glm::vec3 normal)
    {
        // cout << "collision\n";
        glm::vec3 v = transform->getPosition() - go->transform->getPosition();
        vel = glm::reflect(vel, glm::normalize(v));
        if (glm::length2(v) < 2.5)
            transform->move((v) / 2.f);
        // vel += v / 2.f;
    }
    SER_FUNC()
    {
        SER(vel);
    }
};
transform2 comp::orbiter;

void newObject(_model &m, _shader &s);
_model cube;
_shader shader;

class orbit : public component
{

public:
    int num_to_spawn = 1'000'000;
    void onStart()
    {
        cube = _model("res/models/cube/cube.obj");
        shader = _shader("res/shaders/model.vert", "res/shaders/model.frag");
        comp::orbiter = transform;
        transform->setPosition(glm::vec3(glm::cos(Time.time / 3.f), 0, glm::sin(Time.time / 3.f)) * 80.f);

        // int to_spawn = num_to_spawn - Transforms.active();
        // for (int i = 0; i < to_spawn; i++)
        // {
        //     auto g = instantiate();
        //     g->addComponent<comp>();
        //     g->addComponent<_renderer>()->set(shader, cube);
        //     g->transform->setPosition(randomSphere() * 500.f);
        // }
    }
    void update()
    {
        transform->setPosition(glm::vec3(glm::cos(Time.time / 3.f), 0, glm::sin(Time.time / 3.f)) * 80.f);

        int to_spawn = num_to_spawn - Transforms.active();
        console::log("to_spawn: " + to_string(to_spawn));
        for (int i = 0; i < to_spawn; i++)
        {
            auto g = instantiate();
            g->addComponent<comp>();
            g->addComponent<_renderer>()->set(shader, cube);
            // g->addComponent<particle_emitter>();
            // g->addComponent<collider>()->setOBB();
            // g->getComponent<collider>()->setLayer(0);
            // g->getComponent<collider>()->dim = glm::vec3(1);
            // g->transform->setPosition(glm::vec3(randf(), 0, randf()) * 100.f);
            g->transform->setPosition(randomSphere() * 500.f);
        }
        // newObject(cube,shader);
        // concurrency::_parallelfor.doWork(to_spawn,[&](int i){
        // parallelfor(to_spawn, newObject(cube, shader););
    }
    SER_FUNC()
    {
        SER(num_to_spawn)
    }
};

class player : public component
{
public:
    static editor *m_editor;
    void update()
    {
        playerPos = m_editor->position;
        console::log("here");

        if (Input.Mouse.getButtonDown(0))
        {
            ImVec2 mp = ImGui::GetMousePos();
            ImVec2 sz = {float(m_editor->c.width), float(m_editor->c.height)};
            cout << "mp: " << mp.x << "," << mp.y << " sz:" << sz.x << "," << sz.y << endl;
            glm::vec2 sz_2 = {sz.x, sz.y};
            sz_2 /= 2.f;

            camera &c = m_editor->c;
            glm::mat3 per = c.getProjection();

            glm::vec3 p = m_editor->position;
            glm::vec3 d = c.screenPosToRay({mp.x, mp.y});

            glm::vec3 res;
            if (terrain::IntersectRayTerrain(p, d, res))
            {

                auto g = instantiate();
                g->addComponent<_renderer>()->set(shader, cube);
                g->transform->setPosition(res);
            }
        }
    }
    SER_FUNC() {}
};

class bomb : public component
{
public:
    int hit = -1;
    glm::vec3 pos;
    void onCollision(collision &col)
    {

        if (!col.g_o->getComponent<bomb>())
        {
            if (!hit)
                pos = transform.getPosition();
            col.this_collider->dim = glm::vec3(10.f);
            hit = 0;
            if (auto k = col.g_o->getComponent<kinematicBody>())
            {
                k->velocity += glm::vec3(0, 100.f, 0); // 100.f / (glm::length2(pos - k->transform.getPosition()) / 10.f) * (glm::normalize(pos - k->transform.getPosition()) + glm::vec3(0,5.f,0));
            }
        }
        // transform.gameObject()->destroy();
    }
    void update()
    {
        if (hit >= 0)
        {
            // transform.setPosition(pos);
            // transform.gameObject()->getComponent<kinematicBody>()->velocity = glm::vec3(0);
            if (hit > 0)
                transform.gameObject()->destroy();
            hit++;
        }
    }
    SER_FUNC() {}
};
REGISTER_COMPONENT(bomb)

class player2 : public component
{
public:
    float speed = 2;
    _camera *c;
    void onStart()
    {
        c = transform.gameObject()->getComponent<_camera>();
        // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        // float h = 0.f;
        // for (int x = -50; x < 50; x++)
        // {
        //     for (int z = -50; z < 50; z++)
        //     {
        //         auto box = instantiate();
        //         box->transform.setPosition(glm::vec3(x * 10, 100 + h, z * 10) + glm::vec3(4.f));
        //         box->addComponent<_renderer>();
        //         box->addComponent<collider>();
        //         box->addComponent<kinematicBody>();
        //         box->addComponent<bomb>();
        //         h += 0.02f;
        //     }
        // }
    }
    void update()
    {
        transform.translate(glm::vec3(1, 0, 0) * (float)(Input.getKey(GLFW_KEY_A) - Input.getKey(GLFW_KEY_D)) * Time.deltaTime * speed);
        transform.translate(glm::vec3(0, 0, 1) * (float)(Input.getKey(GLFW_KEY_W) - Input.getKey(GLFW_KEY_S)) * Time.deltaTime * speed);
        transform.translate(glm::vec3(0, 1, 0) * (float)(Input.getKey(GLFW_KEY_SPACE) - Input.getKey(GLFW_KEY_LEFT_SHIFT)) * Time.deltaTime * speed);
        // transform->rotate(glm::vec3(0, 0, 1), (float)(Input.getKey(GLFW_KEY_Q) - Input.getKey(GLFW_KEY_E)) * -Time.deltaTime);

        transform.rotate(glm::vec3(0, 1, 0), Input.Mouse.getX() * Time.unscaledDeltaTime * c->c->fov / glm::radians(80.f) * -0.4f);
        transform.rotate(glm::vec3(1, 0, 0), -Input.Mouse.getY() * Time.unscaledDeltaTime * c->c->fov / glm::radians(80.f) * -0.4f);

        transform.setRotation(glm::quatLookAtLH(transform.getRotation() * glm::vec3(0, 0, 1), glm::vec3(0, 1, 0)));

        c->c->fov -= Input.Mouse.getScroll() * glm::radians(1.f);
        c->c->fov = glm::clamp(c->c->fov, glm::radians(5.f), glm::radians(80.f));

        if (Input.getKeyDown(GLFW_KEY_R))
        {
            speed *= 2;
        }
        if (Input.getKeyDown(GLFW_KEY_F))
        {
            speed /= 2;
        }

        playerPos = transform.getPosition();

        // if (Input.Mouse.getButtonDown(0))
        // {
        //     // glm::vec3 r;
        //     // // transform.gameObject()->getComponent<_camera>()->c->screenPosToRay();
        //     // if(raycast(transform.getPosition(),transform.forward(),r)){
        //     //     auto box =  instantiate();
        //     //     box->transform.setPosition(r);
        //     //     box->addComponent<_renderer>();
        //     // }
        //     auto box = instantiate();
        //     box->transform.setPosition(transform.getPosition());
        //     box->addComponent<_renderer>();
        //     box->addComponent<collider>();
        //     box->addComponent<kinematicBody>()->velocity = transform.forward() * 50.f;
        //     // box->addComponent<bomb>();
        // }
        // if (Input.Mouse.getButtonClicked(1))
        // {

        //     auto box = instantiate();
        //     box->transform.setPosition(transform.getPosition());
        //     box->addComponent<_renderer>();
        //     box->addComponent<collider>();
        //     box->addComponent<kinematicBody>()->velocity = transform.forward() * 50.f;
        //     box->addComponent<bomb>();
        // }
    }
    SER_FUNC() {}
};

editor *player::m_editor;

// REGISTER_COMPONENT(player);
REGISTER_COMPONENT(player2);
REGISTER_COMPONENT(orbit);
REGISTER_COMPONENT(comp);

void init_my_project()
{
    cube = _model("res/models/cube/cube.obj");
    _model nano = _model("res/models/nanosuit/nanosuit.obj");
    nano.meta()->name = "nanosuit";

    shader = _shader("res/shaders/model.vert", "res/shaders/model.frag");
    _shader lamp("res/shaders/model.vert", "res/shaders/lamp.frag");
    _shader terrainShader("res/shaders/terrain.vert", "res/shaders/model.frag");
    terrainShader.meta()->name = "terrainShader";
    auto orbiter = _instantiate();
    orbiter->_addComponent<_renderer>()->set(lamp, cube);
    orbiter->_addComponent<orbit>();
    orbiter->transform->setScale(glm::vec3(6.f));
    orbiter->_addComponent<collider>();
    orbiter->getComponent<collider>()->setLayer(0);
    orbiter->getComponent<collider>()->setOBB();

    comp::orbiter = orbiter->transform;

    rootGameObject->_addComponent<terrain>()->shader = terrainShader;
}

class missile final : public component
{
public:
    // rigidBody *rb;
    vec3 vel;
    // bullet b;
    audio explosionSound;
    emitter_prototype_ exp;
    float explosion_size = 1.f;
    bool playSound = false;
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
    void onCollision(collision &col)
    {
        glm::vec3 normal = col.normal;
        if (length(normal) == 0)
            normal = randomSphere();
        exp.burst(transform.getPosition(), normal, transform->getScale() * explosion_size, 50);
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
    SER_FUNC()
    {
        SER(vel);
        SER(exp);
        SER(explosionSound)
        SER(explosion_size)
        SER(playSound)
    }
};
REGISTER_COMPONENT(missile)

struct gun_prototype
{
    vector<vec3> barrels = {vec3(0, 0, 0)};
    game_object_prototype ammo;
    float rof;
    float speed;
    float dispersion;
    float size = 1;
    void r()
    {
        RENDER(barrels);
        RENDER(ammo);
        RENDER(rof);
        RENDER(speed);
        RENDER(dispersion);
        RENDER(size);
    }
};
void renderEdit(const char *name, gun_prototype &gp)
{
    gp.r();
}

namespace YAML
{
    template <>
    struct convert<gun_prototype>
    {
        static Node encode(const gun_prototype &rhs)
        {
            Node node;
            ENCODE_PROTO(ammo)
            ENCODE_PROTO(rof)
            ENCODE_PROTO(speed)
            ENCODE_PROTO(dispersion)
            ENCODE_PROTO(size)
            return node;
        }
        static bool decode(const Node &node, gun_prototype &rhs)
        {
            DECODE_PROTO(ammo)
            DECODE_PROTO(rof)
            DECODE_PROTO(speed)
            DECODE_PROTO(dispersion)
            DECODE_PROTO(size)
            return true;
        }
    };
}
class armory final : public component
{
public:
    vector<gun_prototype> guns;

    SER_FUNC()
    {
        SER(guns);
    }
};
REGISTER_COMPONENT(armory);

class gun final : public component
{
    float lastFire;

public:
    // game_object_prototype ammo;
    // float rof;
    // float speed;
    // float dispersion;
    // float size = 1;
    int armory_id;
    void onStart()
    {
        lastFire = Time.time;
    }
    // void setBarrels(vector<vec3> b)
    // {
    //     barrels = b;
    // }
    bool isReloaded()
    {
        armory *gunid = COMPONENT_LIST(armory)->get(0);
        gun_prototype *g = &gunid->guns[0];
        return Time.time - lastFire > 1.f / g->rof;
    }
    bool fire()
    {
        armory *gunid = COMPONENT_LIST(armory)->get(0);
        gun_prototype *g = &gunid->guns[0];

        // reload += rof * Time.deltaTime;
        if ((Time.time - lastFire) * g->rof > 1.f)
        {
            float x = Time.deltaTime * g->rof;
            float reload = 1; //std::max((float)(int)x, 1.f);
            float r;
            // if(reload > x)
            // 	r = Time.deltaTime * (reload / x);
            // else
            r = Time.deltaTime * (1 - (x - reload));
            lastFire = Time.time - r;
            // for (int i = 0; i < (int)reload; i++)
            // {
            for (auto &j : g->barrels)
            {
                game_object *go = instantiate(g->ammo);
                go->transform->setScale(vec3(g->size));
                go->transform->setPosition(transform->getPosition() + vec3(toMat4(transform->getRotation()) * scale(transform->getScale()) * vec4(j, 1)));
                // go->getComponent<physicsObject>()->init((transform->forward() + randomSphere() * dispersion) * speed);
                go->getComponent<missile>()->vel = (transform->forward() + randomSphere() * g->dispersion) * g->speed;
            }
            // }
            // numCubes.fetch_add((int)reload);
            // reload -= (int)reload;
            return true;
        }
        return false;
        // return true;
    }
    SER_FUNC()
    {
        SER(armory_id)
        // SER(rof)
        // SER(speed)
        // SER(dispersion)
        // SER(barrels)
        // SER(ammo)
    }
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
    SER_FUNC()
    {
        SER(speed)
        SER(axis)
    }
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
    // float t_angles[3];
    // float g_angles[3][2];
    array<float, 3> t_angles;
    array<array<float, 2>, 3> g_angles;
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
            if (guns.id == -1)
                this->onStart();
            barrels = guns->gameObject()->getComponent<gun>();
        }
        armory *gunid = COMPONENT_LIST(armory)->get(barrels->armory_id);
        gun_prototype *g = &gunid->guns[0];
        return g->rof;
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
        // muzzelFlash = getNamedEmitterProto("muzzelFlash");
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
                // getNamedEmitterProto("shockWave").burst(transform->getPosition(), guns->forward(), vec3(0.2), 60);
                // sound->play();
                // muzzelSmoke.burst(guns->forward() * guns->getScale() * 5.3f + guns->getPosition(),guns->forward(),17);
                return true;
            }
        }
        return false;
    }
    //UPDATE(_turret,update);
    SER_FUNC()
    {
        if (x == ser_mode::edit_mode && ImGui::Button("init"))
        {
            vector<vec3> barrels = {vec3(-.56, 0, 2.3), vec3(0, 0, 2.3), vec3(0.56, 0, 2.3)};

            auto g = transform.getChildren().front().gameObject()->getComponent<gun>();
            // g->setBarrels(barrels);
            // // g->ammo = ammo_proto; //bullets["bomb"].proto;
            // g->rof = 1.f / 4.f;
            // g->dispersion = 0.01f;
            // g->speed = 500;

            setTarget(target);
            t_angles[0] = radians(-135.f);
            t_angles[1] = radians(0.f);
            t_angles[2] = radians(135.f);

            g_angles[0][0] = radians(-80.f);
            g_angles[0][1] = radians(20.f);
            g_angles[1][0] = radians(-80.f);
            g_angles[1][1] = radians(3.f);
            g_angles[2][0] = radians(-80.f);
            g_angles[2][1] = radians(20.f);
            turret_speed = glm::radians(100.f);
            gun_speed = glm::radians(100.f);
        }
        SER(target);
        SER(gun_speed)
        SER(turret_speed)
        SER(muzzelFlash)
        // SER(guns)
        SER(t_angles)
        SER(g_angles)
        // SER(turret_angle)
        // SER(guns_angle)
    }
    // SER_HELPER()
    // {
    //     SER_BASE(component);
    //     ar &target &gun_speed &turret_speed &muzzelFlash &guns &t_angles &g_angles &turret_angle &guns_angle; // & forward & under;
    // }
};
REGISTER_COMPONENT(_turret)

class gunManager final : public component
{

    vector<_turret *> turrets;
    int curr = 0;
    double lastFire = 0;
    double rof;

public:
    void fire()
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
            int i_ = 0;
            while (true)
            {
                bool gunsCanFire = false;
                for (auto &i : turrets)
                {
                    if (i->reloaded() && i->onTarget())
                    {
                        // gunsCanFire = true;
                        i_++;
                        i->fire();
                        curr = (curr + 1) % turrets.size();
                        if (i_ >= (int)reload)
                            return;
                    }
                }
                if (!gunsCanFire)
                    return;
            }
        }
        // t = Time.time;
        // while (Time.time + Time.deltaTime > t)
        // {
        //     bool gunsCanFire = false;
        //     for (auto &i : turrets)
        //     {
        //         if (i->reloaded() && i->onTarget())
        //         {
        //             gunsCanFire = true;
        //             t += rof;
        //             i->fire();
        //             curr = (curr + 1) % turrets.size();
        //             // break;
        //         }
        //     }
        //     if (!gunsCanFire)
        //         break;
        // }
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
        rof = 1.f / ((1.f / turrets[0]->getRateOfFire()) / turrets.size());
    }
    void reset()
    {
        turrets.clear();
        onStart();
    }

public:
    void onEdit()
    {
        if (ImGui::Button("reset"))
        {
            reset();
        }
    }
    //UPDATE(gunManager,update);

    SER_FUNC()
    {
    }
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
    SER_FUNC()
    {
        SER(shouldFire)
    }
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
        playerPos = transform.getPosition();
        vel -= vel * 0.4f * Time.deltaTime;
        vel += transform->forward() * accel * 0.4f * Time.deltaTime;
        accel = glm::clamp(accel, -maxReverse, maxForward);
        if (length(vel) > maxForward)
        {
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

    void onCollision(collision &col)
    {

        // getNamedEmitterProto("shockWave").burst(point, transform->forward(), vec3(0.5), 25);
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
    SER_FUNC()
    {
        SER(maxReverse);
        SER(maxForward);
        SER(thrust);
        SER(rotationSpeed);
    }
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
    SER_FUNC()
    {
        SER(t)
    }
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
    // vector<gun *> guns;
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
    transform2 ship_t;

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
        // list<transform2> vt = transform->getParent()->getParent()->getChildren();
        // transform2 ship_t;
        // for (auto &i : vt)
        // {
        //     if (i->name() == "ship")
        //         ship_t = i;
        // }

        ship = ship_t->gameObject()->getComponent<_ship>();
        gm = ship_t->gameObject()->getComponent<gunManager>();
        // rb = transform->gameObject()->getComponent<rigidBody>();
        // guns = transform->gameObject()->getComponents<gun>();
        // bomb = bullets["bomb"];
        // guns[0]->ammo = ammo_proto;
        // guns[0]->rof = 3'000 / 60;
        // guns[0]->dispersion = 0.3f;
        // guns[0]->speed = 200;
        // // laser = bullets["laser"];
        // // guns[1]->ammo = bullets["laser"].proto;
        // guns[1]->rof = 1000 / 60;
        // guns[1]->dispersion = 0;
        // guns[1]->speed = 30000;
        // guns[1]->size = 20;
        // guns[0]->setBarrels({vec3(0.f, -10.f, 45.f)});

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
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        cam = transform->gameObject()->getComponent<_camera>();
        fov = cam->c->fov;
    }
    void update()
    {

        // rb->gravity = false;
        float _80 = radians(80.f);
        transform->getParent()->rotate(inverse(transform->getParent()->getRotation()) * vec3(0, 1, 0), Input.Mouse.getX() * Time.unscaledDeltaTime * rotationSpeed * fov / _80 * -0.01f);
        transform->getParent()->rotate(vec3(1, 0, 0), Input.Mouse.getY() * Time.unscaledDeltaTime * rotationSpeed * fov / _80 * 0.01f);

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
        if (Input.Mouse.getButtonDown(GLFW_MOUSE_BUTTON_LEFT))
        {

            // 	for(int i = 0; i <= Time.deltaTime * 100; i++){
            // 		numBoxes++;
            // 		auto g = new game_object(*physObj);
            // 		vec3 r = randomSphere() * 2.f * randf() + transform->getPosition() + transform->forward() * 12.f;
            // 		physObj->getComponent<physicsObject>()->init(r.x,r.y,r.z, transform->forward() * 30.f + randomSphere()*10.f);
            // 	}
            // guns[0]->fire();
            gm->fire();
            // cout << "FIRE" << endl;
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
    SER_FUNC()
    {
        SER(speed)
        SER(ship_t)
    }
};
REGISTER_COMPONENT(player_sc)

class player_sc3 : public component
{
public:
    float speed = 3.f;
    bool cursorReleased = false;
    float fov = 80;
    // gui::window *info;
    // gui::text *fps;
    vector<gun *> guns;
    game_object_prototype ammo_proto;

    void onStart()
    {
        // info = new gui::window();
        // fps = new gui::text();
        // info->adopt(fps);
        // info->name = "game info";

        // ImGuiWindowFlags flags = 0;
        // flags |= ImGuiWindowFlags_NoTitleBar;
        // flags |= ImGuiWindowFlags_NoMove;
        // flags |= ImGuiWindowFlags_NoResize;
        // info->flags = flags;

        // info->pos = ImVec2(20, 20);
        // info->size = ImVec2(200, 150);

        guns = transform->gameObject()->getComponents<gun>();
        // bomb = bullets["bomb"];
        // guns[0]->ammo = ammo_proto; //bullets["bomb"].proto;
        // guns[0]->rof = 3'000 / 60;
        // guns[0]->dispersion = 0.3f;
        // guns[0]->speed = 200;
        // guns[0]->setBarrels({vec3(-1.1, 0.4, 0.5)});
    }
    void update()
    {
        // fps->contents = "fps: " + to_string(1.f / Time.unscaledSmoothDeltaTime);

        transform->translate(glm::vec3(1, 0, 0) * (float)(Input.getKey(GLFW_KEY_A) - Input.getKey(GLFW_KEY_D)) * Time.deltaTime * speed);
        transform->translate(glm::vec3(0, 0, 1) * (float)(Input.getKey(GLFW_KEY_W) - Input.getKey(GLFW_KEY_S)) * Time.deltaTime * speed);
        transform->rotate(vec3(0, 1, 0), Input.Mouse.getX() * Time.unscaledDeltaTime * fov / 80 * -0.4f);
        transform->rotate(vec3(1, 0, 0), Input.Mouse.getY() * Time.unscaledDeltaTime * fov / 80 * -0.4f);
        vec3 pos = transform->getPosition();
        // pos.y = getTerrain(pos.x, pos.z)->getHeight(pos.x, pos.z).height + 1.8;
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
        if (Input.Mouse.getButtonDown(GLFW_MOUSE_BUTTON_LEFT))
        {
            guns[0]->fire();
        }
    }
    void onEdit() {}
    SER_FUNC()
    {
        SER(speed);
        SER(cursorReleased);
        SER(fov);
    }
};
REGISTER_COMPONENT(player_sc3)
class sun_sc : public component
{

public:
    SER_FUNC()
    {
        SER(distance)
        SER(day_cycle)
    }
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