#pragma once
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
#include <functional>

#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "camera.h"

// #include "_rendering/_renderer.h"

struct editor
{
    camera c;
    glm::vec3 position{0};
    glm::quat rotation{1,0,0,0};
    float speed = 4;

    void translate(glm::vec3 v);
    void rotate(glm::vec3 axis, float angle);
    void update();
};

void renderFunc(camera& c, GLFWwindow* window);

extern GLFWwindow* window;
extern bool renderRunning;
// extern std::queue<std::shared_ptr<std::function<void()>>> renderJobs;
void renderLoop();

extern _shader _quadShader;
extern _shader matProgram;
void updateTiming();
void initGL();
void initiliazeStuff();
void copyTransforms();
void copyRenderers();
void updateTransforms();
void updateRenderers();
