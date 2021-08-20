#include "../game_engine.h"

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
    SER(vel);
    SER_END
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
    SER(num_to_spawn)
    SER_END
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
            ImVec2 sz = {m_editor->c.width, m_editor->c.height};
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
    SER_FUNC()
    SER_END
};

class bomb : public component
{
public:
    int hit = -1;
    glm::vec3 pos;
    void onCollision(collision& col)
    {
        
        if(!col.g_o->getComponent<bomb>()){
            if(!hit)
                pos = transform.getPosition();
                col.this_collider->dim = glm::vec3(10.f);
            hit = 0;
            if(auto k = col.g_o->getComponent<kinematicBody>()){
                k->velocity += glm::vec3(0,100.f,0);// 100.f / (glm::length2(pos - k->transform.getPosition()) / 10.f) * (glm::normalize(pos - k->transform.getPosition()) + glm::vec3(0,5.f,0));
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
    SER_FUNC()
    SER_END
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
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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

        if (Input.Mouse.getButtonDown(0))
        {
            // glm::vec3 r;
            // // transform.gameObject()->getComponent<_camera>()->c->screenPosToRay();
            // if(raycast(transform.getPosition(),transform.forward(),r)){
            //     auto box =  instantiate();
            //     box->transform.setPosition(r);
            //     box->addComponent<_renderer>();
            // }
            auto box = instantiate();
            box->transform.setPosition(transform.getPosition());
            box->addComponent<_renderer>();
            box->addComponent<collider>();
            box->addComponent<kinematicBody>()->velocity = transform.forward() * 50.f;
            // box->addComponent<bomb>();
        }
        if (Input.Mouse.getButtonClicked(1))
        {

            auto box = instantiate();
            box->transform.setPosition(transform.getPosition());
            box->addComponent<_renderer>();
            box->addComponent<collider>();
            box->addComponent<kinematicBody>()->velocity = transform.forward() * 50.f;
            box->addComponent<bomb>();
        }
    }
    SER_FUNC()
    SER_END
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