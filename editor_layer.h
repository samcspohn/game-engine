#pragma once
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
#include <thread>
#include <atomic>
#include <filesystem>
#include <tbb/tbb.h>
#include <mutex>

#include "imgui/imgui.h"
#include "imgui/guizmo/ImGuizmo.h"

#include "_rendering/Model.h"
#include "_rendering/Shader.h"

#include "gpu_vector.h"
#include "Input.h"
#include "components/game_object.h"
#include "_rendering/_renderer.h"
#include "_rendering/camera.h"
#include "helper1.h"
// #include "physics.h"
// #include "particles.h"
// #include "Components/physics.h"
// #include "audio.h"
// #include "gui.h"
#include "_rendering/gpu_sort.h"
#include "renderthread.h"



extern game_object *rootGameObject;
extern tbb::concurrent_queue<function<void()> *> mainThreadWork;

void save_level(const char *filename);
void rebuildGameObject(componentStorageBase *base, int i);

void load_level(const char *filename);
// inspectable *inspector = 0;
// unordered_map<int, bool> selected;

// int offset;
// ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow; // | ImGuiTreeNodeFlags_OpenOnDoubleClick;
// void renderTransform(transform2 t);
extern string working_file;

void editorLayer(GLFWwindow *window, editor *m_editor);
void endEditor();
void dockspaceBegin(GLFWwindow* window, editor* m_editor);
void dockspaceEnd();
extern tbb::concurrent_queue<glm::vec3> floating_origin;
extern atomic<bool> transformsBuffered;
void renderThreadFunc();
extern mutex transformLock;

bool guiRayCast(glm::vec3 p, glm::vec3 d);
bool isGameRunning();
void StartComponents(componentStorageBase *cl);