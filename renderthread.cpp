#include "renderthread.h"
#include <string>
// #include <omp.h>
#include <thread>
#include "Transform.h"
// #include "_rendering/_renderer.h"
#include "gpu_sort.h"
#include "Input.h"
#include "concurrency.h"
#include "particles/particles.h"
#include "batchManager.h"
#include "lighting/lighting.h"

void renderFunc(camera &c, GLFWwindow *window)
{
    float ratio;
    int width, height;
    // mat4x4 m, p, mvp;

    c.render(window);
    // glfwGetFramebufferSize(window, &width, &height);
    // ratio = width / (float)height;

    // glViewport(0, 0, width, height);
    // glClearColor(1.f, .5f, .3f, 1.f);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // glEnable(GL_DEPTH_TEST);
    // glDepthFunc(GL_LESS);
    // glDepthMask(GL_TRUE);
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_BACK);

    // shader.meta()->shader->use();
    // glm::mat4 pers = glm::perspective(glm::radians(80.f), 16.f / 9.f, 1.f, 1000.f);
    // shader.meta()->shader->setMat4("perspective", pers);
    // glm::mat4 view = glm::lookAt(glm::vec3(0), glm::vec3(0, 0, 1), glm::vec3(0, 1, 0));
    // view = glm::translate(glm::vec3(0)) * view;
    // shader.meta()->shader->setMat4("view", view);
    // for (auto &m : cube.meta()->model->meshes)
    // {
    //     glBindVertexArray(m->VAO);
    //     for (int i = 0; i < _renderers.size(); i++)
    //     {
    //         if (_renderers.valid[i])
    //         {
    //             glm::mat4 mod = _renderers.data[i].transform.getModel();
    //             shader.meta()->shader->setMat4("model", mod);
    //             glDrawElementsInstanced(GL_TRIANGLES, m->indices.size(), GL_UNSIGNED_INT, 0,1);
    //             // glDrawElements(GL_TRIANGLES, m->indices.size(), GL_UNSIGNED_INT, 0);
    //         }
    //     }
    //     glBindBuffer(GL_ARRAY_BUFFER, 0);
    //     glBindVertexArray(0);
    // }
}

vector<int> renderCounts = vector<int>(concurrency::numThreads);
vector<vector<vector<GLint>>> transformIdThreadcache;
vector<vector<glm::vec3>> positionsToBuffer;
vector<vector<glm::quat>> rotationsToBuffer;
vector<vector<glm::vec3>> scalesToBuffer;
unordered_map<int, vector<vector<__renderer>>> rendererThreadCache;
// mutex render_lock;

void initiliazeStuff()
{
    transformIdThreadcache = vector<vector<vector<int>>>(concurrency::numThreads, vector<vector<int>>(3));
    positionsToBuffer = vector<vector<glm::vec3>>(concurrency::numThreads);
    rotationsToBuffer = vector<vector<glm::quat>>(concurrency::numThreads);
    scalesToBuffer = vector<vector<glm::vec3>>(concurrency::numThreads);
    //#######################################################################

    // shadowShader = new Shader("res/shaders/directional_shadow_map.vert", "res/shaders/directional_shadow_map.frag", false);
    // OmniShadowShader = new Shader("res/shaders/omni_shadow_map.vert", "res/shaders/omni_shadow_map.geom", "res/shaders/omni_shadow_map.frag", false);
    GPU_MATRIXES = new gpu_vector_proxy<matrix>();

    __RENDERERS_in = new gpu_vector_proxy<__renderer>();
    // __RENDERERS_in->ownStorage();

    __renderer_offsets = new gpu_vector<GLuint>();
    __renderer_offsets->ownStorage();
    __rendererMetas = new gpu_vector<__renderMeta>();
    __rendererMetas->ownStorage();
    initTransform();

    initParticles();
    particle_renderer::init();
    lighting::init();

    // _atomics = new gpu_vector<uint>();
    _block_sums = new gpu_vector<GLuint>();
    _histo = new gpu_vector<GLuint>();
    matProgram = unique_ptr<Shader>(new Shader("res/shaders/transform.comp"));
}

unique_ptr<Shader> matProgram;

void copyT(int id, int rnumThreads)
{
    int numt = rnumThreads;
    int step = Transforms.size() / rnumThreads;
    int i = step * id;

    transformIdThreadcache[id][0].clear(); // pos
    transformIdThreadcache[id][1].clear(); // scl
    transformIdThreadcache[id][2].clear(); // rot

    positionsToBuffer[id].clear();
    rotationsToBuffer[id].clear();
    scalesToBuffer[id].clear();

    auto from = Transforms.updates.begin() + step * id;
    auto to = from + step;

    // transformIdThreadcache[id].reserve(step + 1);
    if (id == rnumThreads - 1)
        to = Transforms.updates.end();

    // int rsrv = std::ceil((float)Transforms.updates.size() / (float)rnumThreads);
    // transformIdThreadcache[id][0].reserve(rsrv); // pos
    // transformIdThreadcache[id][1].reserve(rsrv); // scl
    // transformIdThreadcache[id][2].reserve(rsrv); // rot

    // positionsToBuffer[id].reserve(rsrv);
    // rotationsToBuffer[id].reserve(rsrv);
    // scalesToBuffer[id].reserve(rsrv);
    while (from != to)
    {
        if (from->pos)
        {
            from->pos = false;
            transformIdThreadcache[id][0].emplace_back(i);
            positionsToBuffer[id].emplace_back(((transform2)i).getPosition());
        }
        if (from->rot)
        {
            from->rot = false;
            transformIdThreadcache[id][1].emplace_back(i);
            rotationsToBuffer[id].emplace_back(((transform2)i).getRotation());
        }
        if (from->scl)
        {
            from->scl = false;
            transformIdThreadcache[id][2].emplace_back(i);
            scalesToBuffer[id].emplace_back(((transform2)i).getScale());
        }
        ++from;
        ++i;
    }
}

void copyTransforms()
{
    int rnumThreads = concurrency::numThreads;

    static rolling_buffer dur = 0;
    timer t;
    t.start();
    if (true) //dur.getAverageValue() > 1.f)
    {
        parallelfor(rnumThreads,
                    copyT(i, rnumThreads);)
    }
    else
    {
        for (int id = 0; id < rnumThreads; ++id)
        {
            copyT(id, rnumThreads);
        }
    }

    dur.add(t.stop());
}
void updateTransforms()
{

    //################################################################
    Shader &transformUpdateShader = *matProgram.get();
    transformUpdateShader.use();
    GPU_TRANSFORMS->grow(Transforms.size());
    transformIds->bindData(6);
    GPU_TRANSFORMS->bindData(0);
    gpu_position_updates->bindData(8);
    gpu_rotation_updates->bindData(9);
    gpu_scale_updates->bindData(10);

    transformUpdateShader.setInt("stage", -2); // positions
    for (int i = 0; i < concurrency::numThreads; i++)
    {
        transformIds->bufferData(transformIdThreadcache[i][0]);
        gpu_position_updates->bufferData(positionsToBuffer[i]);
        transformUpdateShader.setUint("num", transformIdThreadcache[i][0].size());
        glDispatchCompute(transformIdThreadcache[i][0].size() / 64 + 1, 1, 1);
        glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
    }

    transformUpdateShader.setInt("stage", -3); // rotations
    for (int i = 0; i < concurrency::numThreads; i++)
    {
        transformIds->bufferData(transformIdThreadcache[i][1]);
        gpu_rotation_updates->bufferData(rotationsToBuffer[i]);
        transformUpdateShader.setUint("num", transformIdThreadcache[i][1].size());
        glDispatchCompute(transformIdThreadcache[i][1].size() / 64 + 1, 1, 1);
        glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
    }

    transformUpdateShader.setInt("stage", -4); // scales
    for (int i = 0; i < concurrency::numThreads; i++)
    {
        transformIds->bufferData(transformIdThreadcache[i][2]);
        gpu_scale_updates->bufferData(scalesToBuffer[i]);
        transformUpdateShader.setUint("num", transformIdThreadcache[i][2].size());
        glDispatchCompute(transformIdThreadcache[i][2].size() / 64 + 1, 1, 1);
        glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
    }
}

void copyR(int id, int rnumThreads, shared_ptr<map<_shader, map<texArray, map<renderingMeta *, Mesh *>>>>& batch)
{
    int __rendererId = 0;
    int count = 0;
    for (auto &i : (*batch))
    {
        rendererThreadCache[count][id].clear();
        for (auto &j : i.second)
        {
            for (auto &k : j.second)
            {
                int step = k.first->ids.size() / rnumThreads;
                int itr = step * id;
                int to = itr + step;
                if (id == rnumThreads - 1)
                {
                    to = k.first->ids.size();
                }
                while (itr != to)
                {
                    if (k.first->ids.getv(itr))
                    {
                        rendererThreadCache[count][id].emplace_back(k.first->ids.get(itr), __rendererId);
                    }
                    ++itr;
                }
                ++__rendererId;
            }
        }
        ++count;
    }
}

void copyRenderers(shared_ptr<map<_shader, map<texArray, map<renderingMeta *, Mesh *>>>>& batch)
{
    int __renderersSize = 0;
    // int __rendererOffsetsSize = 0;
    __renderer_offsets->storage->clear();
    __rendererMetas->storage->clear();
    __renderMeta rm;
    // rm.min = 0;
    // rm.max = 1e32f;

    int count = 0;
    for (auto &i : (*batch))
    {
        rendererThreadCache[count].resize(concurrency::numThreads);
        ++count;
        for (auto &j : i.second)
        {
            for (auto &k : j.second)
            {
                __renderer_offsets->storage->push_back(__renderersSize);
                rm.radius = k.first->m.meta()->radius;
                rm.isBillboard = k.first->isBillboard;
                rm.min = k.first->minRadius;
                rm.max = k.first->maxRadius;
                __rendererMetas->storage->push_back(rm);
                __renderersSize += k.first->ids.active();
            }
        }
    }

    __RENDERERS_in_size = __renderersSize;
    int rnumThreads = concurrency::numThreads;
    static rolling_buffer dur = 0;
    timer t;
    t.start();
    parallelfor(rnumThreads,
                copyR(i, rnumThreads, batch););
    dur.add(t.stop());
}
void updateRenderers()
{

    //##########################################################
    // __RENDERERS_in->bufferData();
    __RENDERERS_in->resize(__RENDERERS_in_size);
    int offset = 0;
    for (auto &i : rendererThreadCache)
    {
        for (auto &j : i.second)
        {
            __RENDERERS_in->bufferData(j, offset, j.size());
            offset += j.size();
        }
    }
}

GLFWwindow *window;
GLdouble lastFrame = 0;
// Shader *shadowShader;
// Shader *OmniShadowShader;

bool hideMouse = true;
// atomic<bool> renderDone(false);
// atomic<bool> renderThreadReady(false);
bool recieveMouse = true;

// editor *m_editor;

// bool gameRunning = false;
// inspectable *inspector = 0;
// ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow; // | ImGuiTreeNodeFlags_OpenOnDoubleClick;
// transform2 selected_transform = -1;
static unordered_map<int, bool> selected_transforms;

/////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////GL WINDOW FUNCTIONS////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

void window_close_callback(GLFWwindow *window)
{
    //if (!time_to_close)
    glfwSetWindowShouldClose(window, GLFW_TRUE);
}
void mouseFrameBegin()
{
    Input.Mouse.xOffset = Input.Mouse.yOffset = Input.Mouse.mouseScroll = 0;
}
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    if (button >= 0 && button < 1024)
    {
        if (action == GLFW_PRESS)
        {
            Input.Mouse.mouseButtons[button] = true;
            // ImGui::GetIO().MouseDown[button] = true;
        }
        else if (action == GLFW_RELEASE)
        {
            Input.Mouse.mouseButtons[button] = false;
            ImGui::GetIO().MouseDown[button] = false;
        }
    }
}

void MouseCallback(GLFWwindow *window, double xPos, double yPos)
{
    if (recieveMouse)
    {
        if (Input.Mouse.firstMouse)
        {
            Input.Mouse.lastX = xPos;
            Input.Mouse.lastY = yPos;
            Input.Mouse.firstMouse = false;
        }

        Input.Mouse.xOffset = xPos - Input.Mouse.lastX;
        Input.Mouse.yOffset = Input.Mouse.lastY - yPos;

        Input.Mouse.lastX = xPos;
        Input.Mouse.lastY = yPos;
    }
    //camera.ProcessMouseMovement(xOffset, yOffset);
}
void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{

    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS)
        {
            Input.keys[key] = true;
            ImGui::GetIO().KeysDown[key] = true;
            Input.keyDowns[key] = true;
        }
        else if (action == GLFW_RELEASE)
        {
            Input.keys[key] = false;
            ImGui::GetIO().KeysDown[key] = false;
        }
    }
}
void ScrollCallback(GLFWwindow *window, double xOffset, double yOffset)
{
    Input.Mouse.mouseScroll = yOffset;
    ImGuiIO &io = ImGui::GetIO();
    io.MouseWheelH += (float)xOffset;
    io.MouseWheel += (float)yOffset;
    //camera.ProcessMouseScroll(yOffset);
}
void window_size_callback(GLFWwindow *window, int width, int height)
{
    glfwSetWindowSize(window, width, height);
}
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    SCREEN_WIDTH = width;
    SCREEN_HEIGHT = height;
    glViewport(0, 0, width, height);
}
// bool eventsPollDone;
void updateTiming()
{
    // Input.resetKeyDowns();
    // mouseFrameBegin();
    glfwPollEvents();
    double currentFrame = glfwGetTime();
    Time.unscaledTime = currentFrame;
    Time.unscaledDeltaTime = currentFrame - lastFrame;
    Time.deltaTime = std::min(Time.unscaledDeltaTime, 1.f / 30.f) * Time.timeScale;
    Time.time += Time.deltaTime;
    Time.timeBuffer.add(Time.unscaledDeltaTime);
    lastFrame = currentFrame;
    Time.unscaledSmoothDeltaTime = Time.timeBuffer.getAverageValue();
    // eventsPollDone = true;
}

void bindWindowCallbacks(GLFWwindow *window)
{
    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    // glfwSetKeyCallback(window, KeyCallback);
    // glfwSetCursorPosCallback(window, MouseCallback);
    // glfwSetScrollCallback(window, ScrollCallback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetWindowCloseCallback(window, window_close_callback);
}
ImFont *font_default;
void initGL()
{
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // glfwWindowHint(GLFW_DOUBLEBUFFER, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "game engine", nullptr, nullptr);
    if (window == nullptr)
    {
        cout << "failed to create GLFW window" << endl;
        glfwTerminate();

        throw EXIT_FAILURE;
    }

    ///////////////////////// GUI ////////////////////////////////

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable docking

    // Setup style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();
    // ImGui::StyleColorsLight();

    font_default = io.Fonts->AddFontDefault();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    IM_ASSERT(ImGui::GetCurrentContext() != NULL && "Missing dear imgui context. Refer to examples app!"); // Exceptionally add an extra assert here for people confused with initial dear imgui setup

    /////////////////////////////////////////////////////////////

    glfwGetFramebufferSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);
    // if (hideMouse)
    //     glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    bindWindowCallbacks(window);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);
    if (glewInit() != GLEW_OK)
    {
        cout << "failed to initialize GLEW" << endl;
        throw EXIT_FAILURE;
    }
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
}

// std::mutex rdr_lck;
// bool renderRunning;
// std::queue<std::unique_ptr<std::function<void()>>> renderJobs;
void renderLoop()
{
    renderThreadID = this_thread::get_id();
    while (renderRunning)
    {
        if (renderJobs.size() > 0)
        {
            rdr_lck.lock();
            auto job = renderJobs.front();
            renderJobs.pop();
            rdr_lck.unlock();
            (*job)();
        }
        else
        {
            std::this_thread::sleep_for(1ns);
        }
    }
}

void editor::translate(glm::vec3 v)
{
    this->position += this->rotation * v;
}
void editor::rotate(glm::vec3 axis, float radians)
{
    this->rotation = glm::rotate(this->rotation, radians, axis);
}

void editor::update()
{

    if (Input.Mouse.getButtonDown(GLFW_MOUSE_BUTTON_2))
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        translate(glm::vec3(1, 0, 0) * (float)(Input.getKey(GLFW_KEY_A) - Input.getKey(GLFW_KEY_D)) * Time.deltaTime * speed);
        translate(glm::vec3(0, 0, 1) * (float)(Input.getKey(GLFW_KEY_W) - Input.getKey(GLFW_KEY_S)) * Time.deltaTime * speed);
        translate(glm::vec3(0, 1, 0) * (float)(Input.getKey(GLFW_KEY_SPACE) - Input.getKey(GLFW_KEY_LEFT_SHIFT)) * Time.deltaTime * speed);
        // transform->rotate(glm::vec3(0, 0, 1), (float)(Input.getKey(GLFW_KEY_Q) - Input.getKey(GLFW_KEY_E)) * -Time.deltaTime);

        rotate(glm::vec3(0, 1, 0), Input.Mouse.getX() * Time.unscaledDeltaTime * c.fov / glm::radians(80.f) * -0.4f);
        rotate(glm::vec3(1, 0, 0), -Input.Mouse.getY() * Time.unscaledDeltaTime * c.fov / glm::radians(80.f) * -0.4f);

        rotation = glm::quatLookAtLH(rotation * glm::vec3(0, 0, 1), glm::vec3(0, 1, 0));

        c.fov -= Input.Mouse.getScroll() * glm::radians(1.f);
        c.fov = glm::clamp(c.fov, glm::radians(5.f), glm::radians(80.f));

        if (Input.getKeyDown(GLFW_KEY_R))
        {
            speed *= 2;
        }
        if (Input.getKeyDown(GLFW_KEY_F))
        {
            speed /= 2;
        }
    }
    else
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    c.update(position, rotation);
}
