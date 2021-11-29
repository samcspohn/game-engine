#include "game_engine.h"
#include "fileWatcher.h"
#include "runtimeCompiler.h"
#include "assetManager.h"

using namespace std;

// mutex toDestroym;
// std::deque<transform2> toDestroy;

REGISTER_COMPONENT(_renderer);
REGISTER_COMPONENT(_camera);
REGISTER_COMPONENT(audiosource)

void doLoopIteration()
{
    timer stopWatch;

    // //UPDATE
    for (auto &i : ComponentRegistry.meta_types)
    {
        componentStorageBase *cb = i.second;
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
        componentStorageBase *cb = i.second;
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
            cameras->get(i)->c->prepRender(*matProgram.get(), window);
        }
    }
}

void renderFunc(editor *ed, rolling_buffer &fps, runtimeCompiler &rc)
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
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        ed->c.width = float(w);
        ed->c.height = float(h);
        ed->c.prepRender(*matProgram.get(), window);
        renderFunc(ed->c, window);
        lineRendererRender(ed->c);
    }

    rc.lock.lock();
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
    // ImGui::PushFont(font_default);

    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    ImGui::EndFrame();

    LineRendererBeginFrame();

    glfwSwapBuffers(window);
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
            int w, h;
            glfwGetFramebufferSize(window, &w, &h);
            c->c->width = float(w);
            c->c->height = float(h);
            c->c->update(c->transform.getPosition(), c->transform.getRotation());
        }
    }
}

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

void printStats()
{
    componentStats.erase("");
    for (map<string, rolling_buffer>::iterator i = componentStats.begin(); i != componentStats.end(); ++i)
    {
        cout << i->first << " -- avg: " << i->second.getAverageValue() << " -- stdDev: " << i->second.getStdDeviation() << endl;
    }
    // cout << "fps : " << 1.f / Time.unscaledSmoothDeltaTime << endl;
    // cout << "entities : " << entities << endl;
}

int main(int argc, char **argv)
{
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
                         shader_manager.init();
                     });
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
    thread fileWatcherThread([&]()
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
                                              }
                                          });
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
    // player::m_editor = ed;

    // toDestroyGameObjects.reserve(10'000);

    int frameCount{0};
    while (!glfwWindowShouldClose(window))
    {

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
                                     }
                                 })
            }
        }

        // ngetc(c);
        time.start();
        {
            lock_guard<mutex> lck(transformLock);
            if (isGameRunning())
            {
                {
                    logger("main loop");
                    doLoopIteration();
                }
                {
                    logger("physics");
                    physicsUpdate(Time.time);
                }
            }
            parallelfor(toDestroyGameObjects.size(), toDestroyGameObjects[i]->_destroy(););
            toDestroyGameObjects.clear();
        }

        if (!isGameRunning())
            ed->update();
        {
            logger("wait for render");
            waitForRenderJob([&]()
                             {
                                 {
                                     logger("update batches");
                                     batchManager::updateBatches();
                                 }
                                 emitterInits.clear();
                                 for (auto &i : emitter_inits)
                                     emitterInits.push_back(i.second);
                                 emitter_inits.clear();
                                 swapBurstBuffer();
                                 updateTiming();
                             });
        }
        if (isGameRunning())
        {
            logger("update cameras");
            updateCameras();
        }
        {
            logger("copy transforms");
            copyTransforms();
        }
        {
            logger("copy renderers");
            copyRenderers();
        }
        // ############################################################
        enqueRenderJob([&]()
                       { renderFunc(ed, fps, rc); });

        fps.add(time.stop());
    }
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
                         glfwTerminate();
                     });

    while (shaders.size() > 0)
    {
        (*shaders.begin())->~Shader();
    }

    renderRunning = false;
    renderthread.join();

    return 1;
}