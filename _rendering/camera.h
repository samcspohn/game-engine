
#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <vector>
// #include "_rendering/_shader.h"
#include "_rendering/renderTexture.h"
#include "_rendering/_renderer.h"
#include <map>
#include <functional>


extern _shader _quadShader;
struct camera{
	camera(GLfloat fov, GLfloat nearPlane, GLfloat farPlane);
	camera();
	~camera();
	
    float ratio;
    int width, height;
	GLfloat fov;
	GLfloat nearPlane;
	GLfloat farPlane;
	bool lockFrustum = false;
	bool inited = false;
	// _frustum f;
	// map<string, map<string, gpu_vector<GLuint>* >> shader_model_culled; // for transforms
	// map<string, map<string, gpu_vector_proxy<matrix> *>> mats;		  // for rendering
	std::vector<GLuint> _rendererOffsets;

	renderTexture gBuffer;
	// _shader _shaderLightingPass;
	// _shader _quadShader;
	// _model lightVolumeModel;

	glm::mat4 view;
	glm::mat4 rot;
	glm::mat4 proj;
	glm::vec2 screen;

	glm::vec3 pos = glm::vec3(0);
	glm::vec3 dir;
	glm::vec3 up;
	glm::vec3 cullpos;
	glm::mat3 camInv;

	glm::vec2 getScreen();
	void prepRender(Shader &matProgram);
	void render(GLFWwindow* window);
	void update(glm::vec3 position,glm::quat rotation);
	glm::mat4 getRotationMatrix();
	glm::mat4 GetViewMatrix();
	glm::mat4 getProjection();
	glm::vec3 screenPosToRay(glm::vec2 mp);
	SER_HELPER(){
		ar &fov &nearPlane &farPlane;
	}
};

extern std::map<int,std::function<void(camera&)>> renderShit;