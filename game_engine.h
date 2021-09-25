#pragma once

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
#include <algorithm>
#include <execution>
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
#include "audio.h"