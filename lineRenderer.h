
#pragma once;
#include <glm/glm.hpp>
#include "gpu_vector.h"
#include "_rendering/Shader.h"
#include "_rendering/camera.h"

void initLineRenderer();

void addLine(glm::vec3 p1, glm::vec3 p2, glm::vec4 color);
void LineRendererBeginFrame();
void lineRendererRender(camera& c);