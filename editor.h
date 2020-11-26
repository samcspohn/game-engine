#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <vector>
#include <list>
#include <string>
#include "texture.h"
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>


using namespace std;


#define RENDER(name)\
    renderEdit(#name,name);

void renderEdit(const char* name, bool& b);
void renderEdit(const char* name, int& i);
void renderEdit(const char* name, float& f);
void renderEdit(const char* name, glm::vec2& v);
void renderEdit(const char* name, glm::vec3& v);
void renderEdit(const char* name, glm::vec4& v);
void renderEdit(const char* name, glm::quat& q);
// void renderEdit(const char* name, bool& b);