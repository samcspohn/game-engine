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
#include "terrain.h"
#include "lineRenderer.h"
#include "batchManager.h"
#include "lighting/lighting.h"
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
_model cube;
_shader shader;
class orbit : public component
{

public:
    void onStart()
    {
        cube = _model("res/models/cube/cube.obj");
        shader = _shader("res/shaders/model.vert", "res/shaders/model.frag");
    }
    void update()
    {
        transform->setPosition(glm::vec3(glm::cos(Time.time / 3.f), 0, glm::sin(Time.time / 3.f)) * 80.f);

        // int to_spawn = 1'0'000 - Transforms.active();
        // for (int i = 0; i < to_spawn; i++)
        // {
        //     auto g = instantiate();
        //     g->addComponent<comp>();
        //     g->addComponent<_renderer>()->set(shader, cube);
        //     g->addComponent<particle_emitter>();
        //     // g->addComponent<collider>()->setOBB();
        //     // g->getComponent<collider>()->setLayer(0);
        //     // g->getComponent<collider>()->dim = glm::vec3(1);
        //     // g->transform->setPosition(glm::vec3(randf(), 0, randf()) * 100.f);
        //     g->transform->setPosition(randomSphere() * 500.f);
        // }
        // newObject(cube,shader);
        // concurrency::_parallelfor.doWork(to_spawn,[&](int i){
        // parallelfor(to_spawn, newObject(cube, shader););
    }
    SER_FUNC()
    SER_END
    COPY(orbit);
};

class player : public component
{
public:
    static editor *m_editor;
    void update()
    {
        playerPos = m_editor->position;
        console::log("here");

        if (Input.Mouse.getButton(0))
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
    COPY(player)
    SER_FUNC()
    SER_END
};
class player2 : public component
{
public:
    float speed = 2;
    _camera *c;
    void onStart()
    {
        c = transform.gameObject()->getComponent<_camera>();
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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
    }
    COPY(player)
    SER_FUNC()
    SER_END
};

editor *player::m_editor;

// REGISTER_COMPONENT(player);
REGISTER_COMPONENT(player2);
REGISTER_COMPONENT(orbit);
REGISTER_COMPONENT(comp);
REGISTER_COMPONENT(_renderer);
REGISTER_COMPONENT(_camera);

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

void renderCameras()
{
    auto cameras = COMPONENT_LIST(_camera);
    for (int i = 0; i < cameras->size(); i++)
    {
        if (cameras->getv(i))
        {
            renderFunc(*cameras->get(i)->c, window);
            lineRendererRender(*cameras->get(i)->c);
        }
    }
}

void prepCameras()
{
    auto cameras = COMPONENT_LIST(_camera);
    for (int i = 0; i < cameras->size(); i++)
    {
        if (cameras->getv(i))
        {
            cameras->get(i)->c->prepRender(*matProgram.meta()->shader.get(), window);
        }
    }
}

void renderFunc(editor *ed, rolling_buffer &fps)
{
    // lock_guard<mutex> lk(transformLock);
    // renderLock.lock();
    updateTransforms();
    updateRenderers();

    uint emitterInitCount = emitterInits.size();
    prepParticles();
    updateParticles(vec3(0), emitterInitCount);
    lightingManager::gpu_pointLights->bufferData();


    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
    dockspaceBegin(window, ed);

    if (isGameRunning())
    {
        prepCameras();
        renderCameras();
    }
    else
    {
        ed->c.prepRender(*matProgram.meta()->shader.get(), window);
        renderFunc(ed->c, window);
        lineRendererRender(ed->c);
    }


    editorLayer(window, ed);
    dockspaceEnd();
    // ImGui::PushFont(font_default);

    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    ImGui::EndFrame();

    LineRendererBeginFrame();

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

void updateCameras()
{
    auto cameras = COMPONENT_LIST(_camera);
    for (int i = 0; i < cameras->size(); i++)
    {
        if (cameras->getv(i))
        {
            _camera *c = cameras->get(i);
            int w,h;
            glfwGetFramebufferSize(window, &w,&h);
            c->c->width = float(w);
            c->c->height = float(h);
            c->c->update(c->transform.getPosition(), c->transform.getRotation());
        }
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
        initLineRenderer();
        ImGui::LoadIniSettingsFromDisk("default.ini");
    });
    initParticles2();
    particle_renderer::init2();
    matProgram = _shader("res/shaders/transform.comp");

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
    // rootGameObject->_addComponent<player>();

    rolling_buffer fps;
    timer time;
    editor *ed = new editor();
    ed->c.fov = glm::radians(80.f);
    ed->c.nearPlane = 0.0001;
    ed->c.farPlane = 100000;
    ed->c.width = 1920;
    ed->c.height = 1080;
    ed->position = glm::vec3(0, 0, -200);

    player::m_editor = ed;

    toDestroyGameObjects.reserve(10'000);

    int frameCount{0};
    while (!glfwWindowShouldClose(window))
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
                updateCameras();
            }
            parallelfor(toDestroyGameObjects.size(), toDestroyGameObjects[i]->_destroy(););
            toDestroyGameObjects.clear();
        }

        waitForRenderJob([&]() {
            batchManager::updateBatches();
            updateTiming();
        });
        if (!isGameRunning())
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
    endEditor();
    rootGameObject->_destroy();
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