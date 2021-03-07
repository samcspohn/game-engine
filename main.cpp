#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
// #include <glm/gtc/swizzle.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <iostream>
#include <assimp/postprocess.h>
#include <chrono>

#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "Input.h"
#include "helper1.h"
#include "Transform.h"
// #include "_renderer.h"
#include "renderthread.h"
using namespace std;

GLFWwindow *window;
GLdouble lastFrame = 0;
// Shader *shadowShader;
// Shader *OmniShadowShader;

bool hideMouse = true;
// atomic<bool> renderDone(false);
// atomic<bool> renderThreadReady(false);
bool recieveMouse = true;

// editor *m_editor;

bool gameRunning = false;
// inspectable *inspector = 0;
// ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow; // | ImGuiTreeNodeFlags_OpenOnDoubleClick;
// transform2 selected_transform = -1;
static unordered_map<int, bool> selected_transforms;

// void editor::translate(glm::vec3 v)
// {
// 	this->position += this->rotation * v;
// }
// void editor::rotate(glm::vec3 axis, float radians)
// {
// 	this->rotation = glm::rotate(this->rotation, radians, axis);
// }

// void editor::update()
// {

// 	if (Input.Mouse.getButton(GLFW_MOUSE_BUTTON_2))
// 	{

// 		translate(glm::vec3(1, 0, 0) * (float)(Input.getKey(GLFW_KEY_A) - Input.getKey(GLFW_KEY_D)) * Time.deltaTime * speed);
// 		translate(glm::vec3(0, 0, 1) * (float)(Input.getKey(GLFW_KEY_W) - Input.getKey(GLFW_KEY_S)) * Time.deltaTime * speed);
// 		translate(glm::vec3(0, 1, 0) * (float)(Input.getKey(GLFW_KEY_SPACE) - Input.getKey(GLFW_KEY_LEFT_SHIFT)) * Time.deltaTime * speed);
// 		// transform->rotate(glm::vec3(0, 0, 1), (float)(Input.getKey(GLFW_KEY_Q) - Input.getKey(GLFW_KEY_E)) * -Time.deltaTime);
// 		rotate(vec3(0, 1, 0), Input.Mouse.getX() * Time.unscaledDeltaTime * c.fov / radians(80.f) * -0.4f);
// 		rotate(vec3(1, 0, 0), Input.Mouse.getY() * Time.unscaledDeltaTime * c.fov / radians(80.f) * -0.4f);

// 		rotation = quatLookAtLH(rotation * vec3(0, 0, 1), vec3(0, 1, 0));

// 		c.fov -= Input.Mouse.getScroll() * radians(5.f);
// 		c.fov = glm::clamp(c.fov, radians(5.f), radians(80.f));

// 		if (Input.getKeyDown(GLFW_KEY_R))
// 		{
// 			speed *= 2;
// 		}
// 		if (Input.getKeyDown(GLFW_KEY_F))
// 		{
// 			speed /= 2;
// 		}
// 	}
// 	c.update(position, rotation);
// }
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
    Input.resetKeyDowns();
    mouseFrameBegin();
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
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetScrollCallback(window, ScrollCallback);
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
    glfwWindowHint(GLFW_DOUBLEBUFFER, GL_TRUE);
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

tbb::concurrent_unordered_set<transform2> toDestroy;

class itr_base
{
public:
    virtual void erase() {}
};

template <typename t>
class itr : public itr_base
{
    typename deque_heap<t>::ref r;

public:
    itr(typename deque_heap<t>::ref re) : r(re) {}
    void erase()
    {
        r._delete();
    }
};

class gObject
{
public:
    map<component *, unique_ptr<itr_base>> components;
};

map<transform2, gObject> gobjects;

class comp : public component
{
public:
    _renderer *r;
    int updateCount = 0;
    void update()
    {
        transform.rotate(glm::vec3(0, 1, 0), glm::radians(50.f));
        updateCount += (rand() % 2 ? 1 : 2);
        if (updateCount > 1000)
        {
            toDestroy.emplace(transform);
        }
    }
};

deque_heap<_renderer> _renderers;
deque_heap<comp> comps;

void newObject(_model& m, _shader& s)
{
    transform2 t = Transforms._new();
    gObject &g = gobjects[t];
    auto refj = comps._new();
    refj->transform = t;
    auto refr = _renderers._new();
    refr->transform = t;
    refr->set(s,m);
    t.setPosition(randomSphere() * 100.f); // + glm::vec3(0,0,100))
    // t.setPosition(glm::vec3(0,0,4));
    // refj->transform = t;
    // auto pj = make_unique<itr_base>(new itr<comp>(refj));
    // unique_ptr<itr_base> pj = make_unique<itr<comp>>(refj);
    g.components.emplace(&(*refj), make_unique<itr<comp>>(refj));
    g.components.emplace(&(*refr), make_unique<itr<_renderer>>(refr));

    // auto refr = _rs._new();
    // auto pr = make_unique<itr_base>(new itr<_renderer>(refr));
    // g.components.emplace(&(*refr),pr);
}

void destroyObject(transform2 &t)
{
    for (auto &j : gobjects[t].components)
    {
        j.first->onDestroy();
        j.second->erase();
    }
    gobjects.erase(t);
    t->_destroy();
}

struct editor{
    camera c;
    glm::vec3 position;
    glm::quat rotation{};
    float speed = 1;

    void translate(glm::vec3 v);
    void rotate(glm::vec3 axis, float angle);
    void update();
};

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

	if (Input.Mouse.getButton(GLFW_MOUSE_BUTTON_2))
	{

		translate(glm::vec3(1, 0, 0) * (float)(Input.getKey(GLFW_KEY_A) - Input.getKey(GLFW_KEY_D)) * Time.deltaTime * speed);
		translate(glm::vec3(0, 0, 1) * (float)(Input.getKey(GLFW_KEY_W) - Input.getKey(GLFW_KEY_S)) * Time.deltaTime * speed);
		translate(glm::vec3(0, 1, 0) * (float)(Input.getKey(GLFW_KEY_SPACE) - Input.getKey(GLFW_KEY_LEFT_SHIFT)) * Time.deltaTime * speed);
		// transform->rotate(glm::vec3(0, 0, 1), (float)(Input.getKey(GLFW_KEY_Q) - Input.getKey(GLFW_KEY_E)) * -Time.deltaTime);
        
		rotate(glm::vec3(0, 1, 0), Input.Mouse.getX() * Time.unscaledDeltaTime * c.fov / glm::radians(80.f) * -0.4f);
		rotate(glm::vec3(1, 0, 0), Input.Mouse.getY() * Time.unscaledDeltaTime * c.fov / glm::radians(80.f) * -0.4f);

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
	c.update(position, rotation);
}
int main(int argc, char **argv)
{


    initGL();
    initiliazeStuff();
    matProgram = Shader("res/shaders/transform.comp");

    _model cube("res/models/cube/cube.obj");
    _shader shader("res/shaders/model.vert", "res/shaders/model.frag");

    rolling_buffer fps;
    timer time;
    editor ed;
    ed.c.fov = glm::radians(80.f);
    ed.c.nearPlane = 1;
    ed.c.farPlane = 1000;

    int frameCount{0};
    while (!glfwWindowShouldClose(window))
    {

        time.start();
        if (frameCount++ % 1000 == 0)
        {
            cout << "##########################" << endl;
            cout << "gobjects " << gobjects.size() << endl;
            cout << "comps " << comps.active() << endl;
            // cout << comps.valid.size() << endl;
            cout << "transforms " << Transforms.active() << endl;
        }
        while (gobjects.size() < 1000)
        {
            newObject(cube,shader);
        }

        for (int i = 0; i < comps.size(); i++)
        {
            if (comps.valid[i])
            {
                comps.data[i].update();
            }
        }
        for (auto &i : toDestroy)
        {
            destroyObject(i);
        }
        toDestroy.clear();
        
        updateTiming();
        ed.update();
        updateRenderers();
        updateTransforms();

        ed.c.prepRender(matProgram);
        renderFunc(ed.c,window);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        // ImGuizmo::BeginFrame();

        ImGui::PushFont(font_default);

        // dockspace();

        ImGui::Begin("info");
        ImGui::Text(("fps: " + std::to_string(1.f / fps.getAverageValue() * 1000)).c_str());
        ImGui::End();
        ImGui::PopFont();
        // Rendering
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        ImGui::EndFrame();

        glfwSwapBuffers(window);
        glfwPollEvents();

        fps.add(time.stop());
    }

    // Cleanup gui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glFlush();
    glfwTerminate();
    return 1;
}