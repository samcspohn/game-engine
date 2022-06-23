#include "game_engine.h"
#include "fileWatcher.h"
#include "runtimeCompiler.h"
#include "assetManager.h"
#include "physics/physics_.h"
// #include <iomanip.h>
// #include <barrier>

#include <iostream>
#include <condition_variable>
#include <thread>
#include <chrono>

struct bullet_box_rb : component
{

    // btRigidBody *bt = 0;
    void onStart()
    {
        // glm::vec3 pos = transform->getPosition();
        // glm::quat rot = transform->getRotation();
        // bt = pm->addBox(glm::vec3(1.f), rot, pos, 1);
    }
    void update()
    {
        // _transform t = getTransform(bt);
        // transform->setPosition(t.position);
        // transform->setRotation(t.rotation);
    }
    void onDestroy()
    {
        // if (bt)
        //     pm->removeRigidBody(bt);
    }

    SER_FUNC()
    {
    }
};
REGISTER_COMPONENT(bullet_box_rb);

struct bullet_ship_rb : component
{

    // btRigidBody *bt = 0;
    PxRigidStatic *pt = 0;
    _model m;
    void onStart()
    {
        glm::vec3 pos = transform->getPosition();
        glm::quat rot = transform->getRotation();

        pt = pm->addPxMesh(pos, &m.mesh(), transform->gameObject());
    }
    void update()
    {
        auto t = pt->getGlobalPose();
        transform->setPosition(*reinterpret_cast<glm::vec3 *>(&t.p));
        transform->setRotation(*reinterpret_cast<glm::quat *>(&t.q));
    }
    void onDestroy()
    {
        if (pt)
            pt->release();
        // if (bt)
        //     pm->removeRigidBody(bt);
    }

    SER_FUNC()
    {
        SER(m)
    }
};
REGISTER_COMPONENT(bullet_ship_rb);


using namespace std;
struct logger
{
    string name;
    timer t;
    logger(string name) : name(name)
    {
        t.start();
    }
    ~logger()
    {
        appendStat(name, t.stop());
    }
};

void doLoopIteration()
{
    timer stopWatch;

    // //UPDATE
    for (auto &i : ComponentRegistry.meta_types)
    {
        componentStorageBase *cb = i.second;
        if (cb->hasUpdate())
        {
            profiler.Begin(("update--" + cb->name).c_str());
            stopWatch.start();
            cb->update();
            appendStat("update--" + cb->name, stopWatch.stop());
            profiler.End(("update--" + cb->name).c_str());
        }
    }
    // LATE //UPDATE
    for (auto &i : ComponentRegistry.meta_types)
    {
        componentStorageBase *cb = i.second;
        if (cb->hasLateUpdate())
        {
            profiler.Begin(("update_late--" + cb->name).c_str());
            stopWatch.start();
            cb->lateUpdate();
            appendStat("update_late--" + cb->name, stopWatch.stop());
            profiler.End(("update_late--" + cb->name).c_str());
        }
    }
}

mutex renderLock;
barrier sync_point(2);
mutex crit;

void renderCameras()
{
    // sync_point.wait();
    auto cameras = COMPONENT_LIST(_camera);
    for (int i = 0; i < cameras->size(); i++)
    {
        if (cameras->getv(i))
        {
            // renderFunc(*cameras->get(i)->c, window,sync_point);
            cameras->get(i)->c->render(window,sync_point);
            // lineRendererRender(*cameras->get(i)->c);
        }
    }
}

void prepCameras(barrier& b)
{
    auto cameras = COMPONENT_LIST(_camera);
    for (int i = 0; i < cameras->size(); i++)
    {
        if (cameras->getv(i))
        {
            cameras->get(i)->c->prepRender(*matProgram.get(), window, b);
        }
    }
}


void renderFunc(editor *ed, rolling_buffer &fps, runtimeCompiler &rc)
{
    {
        updateTransforms();
        updateRenderers();

        prepParticles();
        lightingManager::gpu_pointLights->bufferData();
        uint emitterInitCount = emitterInits.size();
        updateParticles(vec3(0), emitterInitCount);
    }
        // crit.lock();
    // sync_point.wait();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
    dockspaceBegin(window, ed);

        // sync_point.wait();
    if (isGameRunning())
    {
        rc.fw.pause = true;
        
        prepCameras(sync_point);
        // sync_point.wait();
        renderCameras();
    }
    else
    {
        // sync_point.wait();
        // crit.lock();

        rc.fw.pause = false;
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        ed->c.width = float(w);
        ed->c.height = float(h);
        ed->c.prepRender(*matProgram.get(), window, sync_point);
        // sync_point.wait();
        renderFunc(ed->c, window, sync_point);
        lineRendererRender(ed->c);
    }
    // sync_point.wait();
    // crit.unlock();

    rc.lock.lock();
    pm->drawDebug();

    editorLayer(window, ed, rc.getCompiling());

    if (rc.getCompiling())
    {
        auto sz = ImGui::GetWindowSize();
        ImGui::SetNextWindowPos({sz.x / 2 - 100, sz.y / 2 - 50}, ImGuiCond_Once);
        ImGui::SetNextWindowSize({200.f, 100.f}, ImGuiCond_Once);
        ImGui::SetWindowFontScale(2);
        ImGui::Begin("cmpiling", 0, ImGuiWindowFlags_NoTitleBar);
        ImGui::Text("Compiling");
        ImGui::End();
        ImGui::SetWindowFontScale(1);
    }
    rc.lock.unlock();
    dockspaceEnd();
    // sync_point.wait();

    // ImGui::PushFont(font_default);

    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    ImGui::EndFrame();

    LineRendererBeginFrame();

    glfwSwapBuffers(window);
    // }
}

// #include <unistd.h>
// #include <fcntl.h>
// #include <stdlib.h>
// #include <termios.h>

// ssize_t ngetc(char &c)
// {
//     return read(0, &c, 1);
// }

void physicsUpdate(float dt)
{

    // for (auto &i : physics_manager::collisionGraph)
    //     physics_manager::collisionLayers[i.first].clear();
    // timer stopWatch;
    static float time = 0;
    time += dt;
    timer t;
    if (time >= (1.f / 30.f))
    {
        pm->gScene->simulate(1.0f / 30.0f);
        pm->gScene->fetchResults(true);
        // Start simulation
        // t.start();
        // appendStat("Physics", t.stop());
        for (auto [_, i] : ComponentRegistry.meta_types)
        {
            // logger(i->getName() + "--late_update");
            if (i->h_fixedUpdate)
            {
                t.start();
                i->fixedUpdate();
                appendStat("update_fixed--" + i->getName(), t.stop());
            }
        }
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
            int w, h;
            glfwGetFramebufferSize(window, &w, &h);
            c->c->width = float(w);
            c->c->height = float(h);
            c->c->update(c->transform.getPosition(), c->transform.getRotation());
        }
    }
}

void printStats()
{
    componentStats.erase("");
    cout.setf(ios::fixed);
    for (map<string, rolling_buffer>::iterator i = componentStats.begin(); i != componentStats.end(); ++i)
    {
        cout << "avg: " << setw(8) << setprecision(5) << i->second.getAverageValue() << " -- stdDev: " << setw(8) << setprecision(5) << i->second.getStdDeviation() << " " << i->first << endl;
    }
    // cout << "fps : " << 1.f / Time.unscaledSmoothDeltaTime << endl;
    // cout << "entities : " << entities << endl;
}

REGISTER_COMPONENT(_renderer);
REGISTER_COMPONENT(_camera);
REGISTER_COMPONENT(audiosource);
REGISTER_COMPONENT(terrain);
REGISTER_COMPONENT(placeholder);
REGISTER_COMPONENT(Light);
REGISTER_COMPONENT(particle_emitter);
REGISTER_COMPONENT(rigidBody);
REGISTER_COMPONENT(collider);
REGISTER_COMPONENT(kinematicBody);

int main(int argc, char **argv)
{
    // system("rm -r ./test_project/runtime");
    pm = new _physicsManager();
    pm->setDebug();

    assetManager asset_manager;
    asset_manager.registerAssetManager({".obj"}, &model_manager);
    FileWatcher fw{"./test_project", std::chrono::milliseconds(500)};
    dlopen(NULL, RTLD_NOW | RTLD_GLOBAL);
    runtimeCompiler rc;
    rc.include.push_back("_rendering");
    rc.include.push_back("components");
    rc.include.push_back("lighting");
    rc.include.push_back("particles");
    rc.include.push_back("physics");
    rc.include.push_back("imgui");
    rc.include.push_back("imgui/imgui-flame-graph");
    rc.include.push_back("PhysX/physx/include");
    rc.include.push_back("PhysX/pxshared/include");

    physics_manager::collisionGraph[-1] = {};
    physics_manager::collisionGraph[0] = {0};
    // physics_manager::collisionGraph[1] = {0, 1};

    root2 = Transforms._new();
    newGameObject(root2);
    rootGameObject = root2->gameObject();

    thread renderthread(renderLoop);
    waitForRenderJob([&]()
                     {
                         initGL();
                         initiliazeStuff();
                         initLineRenderer();
                         ImGui::LoadIniSettingsFromDisk("default.ini");
                         model_manager.init();
                         shader_manager.init(); });
    initParticles2();
    particle_renderer::init2();

    {
        loadAssets();
        YAML::Node assets_node = YAML::LoadFile("assets.yaml");
        fw.getFileData(assets_node["file_meta"]);
        try
        {
            rc.fw.getFileData(assets_node["scripts"]);
            // rc.initLoadedScripts();
            rc.run("./test_project");
        }
        catch (YAML::Exception e)
        {
        }
        if (working_file != "")
        {
            load_level(working_file.c_str());
        }
    }

    // Start monitoring a folder for changes and (in case of changes)
    // run a user provided lambda function
    thread fileWatcherThread(
        [&]()
        {
            fw.start([&](std::string path_to_watch, FileStatus status) -> void
                     {
                // Process only regular files, all other file types are ignored
                if (!std::filesystem::is_regular_file(std::filesystem::path(path_to_watch)) && status != FileStatus::erased)
                {
                    return;
                }

                switch (status)
                {
                case FileStatus::created:
                    std::cout << "File created: " << path_to_watch << '\n';
                    break;
                case FileStatus::modified:
                    std::cout << "File modified: " << path_to_watch << '\n';
                    break;
                case FileStatus::erased:
                    std::cout << "File erased: " << path_to_watch << '\n';
                    break;
                default:
                    std::cout << "Error! Unknown file status.\n";
                }
                //   if (path_to_watch.find(".obj") != -1)
                //   {
                //       _model m(path_to_watch);
                //       m.meta()->name = path_to_watch.substr(path_to_watch.find_last_of('/') + 1);
                //   }
                if (path_to_watch != "assets.yaml")
                {
                    asset_manager.load(path_to_watch);
                    YAML::Node assets = YAML::LoadFile("assets.yaml");
                    assets["file_meta"] = fw.getFileData();
                    ofstream("assets.yaml") << assets;
                } });
        });

    // rootGameObject->_addComponent<player>();

    rolling_buffer fps;
    timer time;
    editor *ed = new editor();
    ed->c.fov = glm::radians(80.f);
    ed->c.nearPlane = 0.0001;
    ed->c.farPlane = 100000;
    ed->c.width = 1920;
    ed->c.height = 1080;
    ed->position = glm::vec3(0, 0, -10);
    int frameCount{0};
    timer t;
    bool first_frame = true;
    while (!glfwWindowShouldClose(window))
    {
        bool sync_hit_early = false;
        profiler.Frame();
        {

            t.start();
            {
                logger("main thread work");
                function<void()> *f;
                while (mainThreadWork.try_pop(f))
                {
                    (*f)();
                    delete f;
                }
            }
            {
                // lock_guard<mutex> lck(transformLock);
                if (rc.getCompilationComplete() && rc.getCompilationSuccess())
                {
                    // sync_point.wait();
                    // sync_hit_early = true;
                    waitForRenderJob([&]()
                                     {
                                     YAML::Node root_game_object_node;
                                     game_object::encode(root_game_object_node, rootGameObject);
                                     root_game_object_node;

                                     rootGameObject->_destroy();
                                     Transforms.clear();

                                     saveAssets();
                                     ////////////////////////////////////////////////////
                                     rc.reloadModules();
                                     ////////////////////////////////////////////////////

                                     // loadAssets();
                                     YAML::Node assets_node = YAML::LoadFile("assets.yaml");
                                     game_object_proto_manager.decode(assets_node);
                                     game_object::decode(root_game_object_node, -1);

                                     rootGameObject = transform2(0)->gameObject();
                                     for (auto &i : ComponentRegistry.meta_types)
                                     {
                                         initComponents(i.second);
                                     } })
                }
            }

            // if (first_frame)
            // {
            //     first_frame = false;
            // }
            // else
            // {
            //     sync_point.wait();
            // }
            time.start();
            {

                profiler.Begin("main");
                lock_guard<mutex> lck(transformLock);
                if (isGameRunning())
                {
                    {
                        // logger("main loop");
                        t.start();
                        doLoopIteration();
                        appendStat("main loop", t.stop());
                    }
                    {
                        // logger("physics");
                        physicsUpdate(Time.deltaTime);
                    }
                }
                else
                {
                    // EDITOR UPDATE
                    for (auto &i : ComponentRegistry.meta_types)
                    {
                        componentStorageBase *cb = i.second;
                        if (cb->hasEditorUpdate())
                        {
                            cb->editorUpdate();
                        }
                    }
                }
                profiler.Begin("deffered destroy");
                parallelfor(toDestroyGameObjects.size(), toDestroyGameObjects[i]->_destroy(););
                toDestroyGameObjects.clear();
                profiler.End("deffered destroy");

                for (auto &[_, i] : ComponentRegistry.meta_types)
                {
                    i->compress();
                }
                profiler.End("main");
            }

            if (!isGameRunning())
                ed->update();
        }
        // if (first_frame || sync_hit_early)
        // {
        //     first_frame = false;
        // }
        // else
        // {
        //     sync_point.wait();
        // }
        {
            logger("wait for render");
            profiler.Begin("wait for render");
            auto job = [&]()
            {
                profiler.End("wait for render");
                updateTiming();
            };
            waitForRenderJob(job);
        }
        {
            // lock_guard<mutex> lck(render_lock);
            {

                emitterInits.clear();
                for (auto &i : emitter_inits)
                    emitterInits.push_back(i.second);
                emitter_inits.clear();
                swapBurstBuffer();
            }
            if (isGameRunning())
            {
                // logger("update cameras");
                profiler.Begin("update cameras");
                t.start();
                updateCameras();
                appendStat("update cameras", t.stop());
                profiler.End("update cameras");
            }
            {
                // logger("copy transforms");
                profiler.Begin("copy transforms");
                copyTransforms();
                appendStat("copy transforms", t.stop());
                profiler.End("copy transforms");
            }
            {
                profiler.Begin("update batches");
                t.start();
                auto batch = batchManager::updateBatches();
                appendStat("update batches", t.stop());
                profiler.End("update batches");

                profiler.Begin("copy renderers");
                copyRenderers(batch);
                appendStat("copy renderers", t.stop());
                profiler.End("copy renderers");
            }
        }
        // ############################################################

        // profiler.Begin("render func");waitForRenderJob   enqueRenderJob
        enqueRenderJob([&]()
                       { renderFunc(ed, fps, rc); });
        // sync_point.wait();

        // profiler.End("ren    der func");
        // this_thread::sleep_for(((1.f / 60.f) - t.stop() / 1000.f) * 1000 * 1ms);
        fps.add(time.stop());
    }

    // sync_point.wait();

    fw.stop();
    saveAssets();

    rc.stop();

    YAML::Node assets = YAML::LoadFile("assets.yaml");
    assets["scripts"] = rc.fw.getFileData();
    ofstream("assets.yaml") << assets;

    fileWatcherThread.join();
    printStats();
    endEditor();
    rootGameObject->_destroy();
    delete ed;
    delete pm;
    renderingManager::destroy();
    shader_manager.destroy();
    model_manager.destroy();
    particle_renderer::end();

    waitForRenderJob([&]()
                     {
                         while (gpu_buffers.size() > 0)
                         {
                             (gpu_buffers.begin()->second)->deleteBuffer();
                         }
                         // Cleanup gui
                         ImGui_ImplOpenGL3_Shutdown();
                         ImGui_ImplGlfw_Shutdown();
                         ImGui::DestroyContext();

                         glFlush();
                         glfwTerminate(); });

    while (shaders.size() > 0)
    {
        (*shaders.begin())->~Shader();
    }

    renderRunning = false;
    renderthread.join();

    return 1;
}