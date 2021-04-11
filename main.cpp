#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>
// #include <glm/gtc/swizzle.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <iostream>
#include <assimp/postprocess.h>
#include <chrono>
#include <algorithm>
#include <execution>
// #include <omp.h>
#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "_serialize.h"
#include "Input.h"
#include "helper1.h"
#include "Transform.h"
// #include "_renderer.h"
// #include "renderthread.h"
#include "editor_layer.h"
// #include <tbb/concurrent_vector.h>
// #include <tbb/spin_mutex.h>
#include "components/game_object.h"
#include "physics/physics.h"
#include "particles/particles.h"

using namespace std;

// mutex toDestroym;
// std::deque<transform2> toDestroy;

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
    COPY(comp);
};
transform2 comp::orbiter;

void newObject(_model &m, _shader &s);
class orbit : public component
{
    _model cube;
    _shader shader;

public:
    void onStart()
    {
        cube = _model("res/models/cube/cube.obj");
        shader = _shader("res/shaders/model.vert", "res/shaders/model.frag");
    }
    void update()
    {
        transform->setPosition(glm::vec3(glm::cos(Time.time / 3.f), 0, glm::sin(Time.time / 3.f)) * 80.f);

        int to_spawn = 1'0'000 - Transforms.active();
        for (int i = 0; i < to_spawn; i++)
        {
            auto g = instantiate();
            g->addComponent<comp>();
            g->addComponent<_renderer>()->set(shader, cube);
            g->addComponent<particle_emitter>();
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
    SER_END
    COPY(orbit);
};

REGISTER_COMPONENT(orbit);
REGISTER_COMPONENT(comp);
REGISTER_COMPONENT(_renderer);

void doLoopIteration()
{
    timer stopWatch;

    // //UPDATE
    for (auto &i : ComponentRegistry.meta_types)
    {
        componentStorageBase *cb = i.second->getStorage();
        if (cb->hasUpdate())
        {
            stopWatch.start();
            cb->update();
            appendStat(cb->name + "--update", stopWatch.stop());
        }
    }
    // LATE //UPDATE
    for (auto &i : ComponentRegistry.meta_types)
    {
        componentStorageBase *cb = i.second->getStorage();
        if (cb->hasLateUpdate())
        {
            stopWatch.start();
            cb->lateUpdate();
            appendStat(cb->name + "--late_update", stopWatch.stop());
        }
    }
}

mutex renderLock;

void renderFunc(editor *ed, rolling_buffer &fps)
{
    // lock_guard<mutex> lk(transformLock);
    // renderLock.lock();
    updateTransforms();
    updateRenderers();

    uint emitterInitCount = emitterInits.size();
    prepParticles();

    ed->c.prepRender(*matProgram.meta()->shader.get());

    updateParticles(vec3(0), emitterInitCount);
    // glFlush();
    renderFunc(ed->c, window);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    // ImGui::PushFont(font_default);

    dockspace(window, ed);

    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    ImGui::EndFrame();

    glfwSwapBuffers(window);
}
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <termios.h>

ssize_t ngetc(char &c)
{
    return read(0, &c, 1);
}

void physicsUpdate(float dt)
{
    for (auto &i : physics_manager::collisionGraph)
        physics_manager::collisionLayers[i.first].clear();
    timer stopWatch;
    static float time = 0;
    time += dt;
    if (time > 1.f / 30.f)
    {

        componentStorage<collider> *cb = COMPONENT_LIST(collider);
        stopWatch.start();

        parallelfor(
            cb->size(),
            if (cb->data.getv(i)) {
                cb->data.get(i)._update();
            });
        appendStat(cb->name + "--update", stopWatch.stop());
        stopWatch.start();
        parallelfor(
            cb->data.size(),
            if (cb->data.getv(i)) {
                cb->data.get(i).midUpdate();
            });
        appendStat(cb->name + "--mid_update", stopWatch.stop());
        stopWatch.start();
        parallelfor(
            cb->size(),
            if (cb->data.getv(i)) {
                cb->data.get(i)._lateUpdate();
            });
        appendStat(cb->name + "--late_update", stopWatch.stop());
        time -= (1.f / 30.f);
    }
}

int main(int argc, char **argv)
{
    physics_manager::collisionGraph[-1] = {};
    physics_manager::collisionGraph[0] = {0};
    // physics_manager::collisionGraph[1] = {0, 1};

    root2 = Transforms._new();
    newGameObject(root2);
    rootGameObject = root2->gameObject();

    thread renderthread(renderLoop);
    waitForRenderJob([&]() {
        initGL();
        initiliazeStuff();
    });
    initParticles2();
    particle_renderer::init2();
    _quadShader = _shader("res/shaders/defLighting.vert", "res/shaders/defLighting.frag");
    matProgram = _shader("res/shaders/transform.comp");

    _model cube("res/models/cube/cube.obj");
    _shader shader("res/shaders/model.vert", "res/shaders/model.frag");
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

    rolling_buffer fps;
    timer time;
    editor *ed = new editor();
    ed->c.fov = glm::radians(80.f);
    ed->c.nearPlane = 1;
    ed->c.farPlane = 100000;
    ed->c.width = 1920;
    ed->c.height = 1080;
    ed->position = glm::vec3(0, 0, -200);

    toDestroyGameObjects.reserve(10'000);

    int frameCount{0};
    while (!glfwWindowShouldClose(window))
    // while (true)
    // struct termios t;
    // tcgetattr(0, &t);
    // t.c_lflag &= ~ICANON;
    // tcsetattr(0, TCSANOW, &t);

    // fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);

    // char c = 0;
    // while (!c)
    {

        function<void()> *f;
        while (mainThreadWork.try_pop(f))
        {
            (*f)();
            delete f;
            /* code */
        }

        // ngetc(c);
        time.start();
        {
            lock_guard<mutex> lck(transformLock);
            if (isGameRunning())
            {
                doLoopIteration();
                physicsUpdate(Time.time);
            }
            parallelfor(toDestroyGameObjects.size(), toDestroyGameObjects[i]->_destroy(););
            toDestroyGameObjects.clear();
        }

        waitForRenderJob([&]() {
            updateTiming();
        });
        ed->update();
        copyTransforms();
        copyRenderers();
        // ############################################################
        enqueRenderJob([&]() {
            emitterInits.clear();
            for (auto &i : emitter_inits)
                emitterInits.push_back(i.second);
            emitter_inits.clear();
            renderFunc(ed, fps);
        });

        fps.add(time.stop());
    }
    delete ed;
    renderingManager::destroy();
    shaderManager::destroy();
    modelManager::destroy();
    particle_renderer::end();

    waitForRenderJob([&]() {
        while (gpu_buffers.size() > 0)
        {
            (gpu_buffers.begin()->second)->deleteBuffer();
        }
        // Cleanup gui
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glFlush();
        glfwTerminate();
    });

    renderRunning = false;
    renderthread.join();

    return 1;
}