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
#include "Model.h"
#include "Shader.h"
#include <thread>

#include "gpu_vector.h"
#include "Input.h"
#include <atomic>
#include "game_object.h"
#include "rendering.h"
#include "camera.h"
// #include "physics.h"
// #include "particles.h"
// #include "Components/physics.h"
#include "audio.h"
#include "gui.h"
#include "gpu_sort.h"
#include "thread"
#include <tbb/tbb.h>
#include <filesystem>
#include <mutex>
namespace fs = std::filesystem;

using namespace std;

extern GLFWwindow *window;
extern GLdouble lastFrame;
extern Shader *shadowShader;
extern Shader *OmniShadowShader;

extern bool hideMouse;
extern atomic<bool> renderDone;
extern atomic<bool> renderThreadReady;
extern bool recieveMouse;

struct editor{
    camera c;
    glm::vec3 position;
    glm::quat rotation{};
    float speed = 1;

    void translate(glm::vec3 v);
    void rotate(glm::vec3 axis, float angle);
    void update();
};
extern editor* m_editor;

/////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////GL WINDOW FUNCTIONS////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

void window_close_callback(GLFWwindow *window);
void mouseFrameBegin();
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void MouseCallback(GLFWwindow *window, double xPos, double yPos);
void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
void ScrollCallback(GLFWwindow *window, double xOffset, double yOffset);
void window_size_callback(GLFWwindow *window, int width, int height);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void updateTiming();
// int frameCounter = 0;

// #include <sched.h>

// void GLAPIENTRY
// MessageCallback(GLenum source,
// 				GLenum type,
// 				GLuint id,
// 				GLenum severity,
// 				GLsizei length,
// 				const GLchar *message,
// 				const void *userParam)
// {
// 	fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
// 			(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
// 			type, severity, message);
// }

extern game_object *rootGameObject;
extern deque<function<void()> *> mainThreadWork;

void save_game(const char *filename);
void rebuildGameObject(componentStorageBase *base, int i);

void load_game(const char *filename);
// inspectable *inspector = 0;
// unordered_map<int, bool> selected;

// int offset;
// ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow; // | ImGuiTreeNodeFlags_OpenOnDoubleClick;
// void renderTransform(transform2 t);
extern string working_file;
void dockspace();
extern tbb::concurrent_queue<glm::vec3> floating_origin;
extern atomic<bool> transformsBuffered;
void renderThreadFunc();
extern mutex transformLock;

bool guiRayCast(vec3 p, vec3 d);