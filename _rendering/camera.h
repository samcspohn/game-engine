
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

struct camera
{
	camera(GLfloat fov, GLfloat nearPlane, GLfloat farPlane);
	camera();
	~camera();

	float ratio;
	int width, height;
	GLfloat fov = glm::radians(80.f);
	GLfloat nearPlane = 0.1f;
	GLfloat farPlane = 1e4f;
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
	glm::vec3 clearColor;

	glm::vec2 getScreen();
	void prepRender(Shader &matProgram, GLFWwindow *window, barrier& b);
	void render(GLFWwindow *window);
	void update(glm::vec3 position, glm::quat rotation);
	glm::mat4 getRotationMatrix();
	glm::mat4 GetViewMatrix();
	glm::mat4 getProjection();
	glm::vec3 screenPosToRay(glm::vec2 mp);
};

class _camera : public component
{
public:
	_camera(GLfloat fov, GLfloat nearPlane, GLfloat farPlane);
	_camera();
	_camera(const _camera &c);
	unique_ptr<camera> c;
	SER_FUNC()
	{
		switch (x)
		{
		case ser_mode::edit_mode:
			renderEdit("fov", c->fov);
			renderEdit("near", c->nearPlane);
			renderEdit("far", c->farPlane);
			renderEdit("clear color", c->clearColor);
			break;
		case ser_mode::read_mode:
			c->fov = node_9738469372465["fov"].as<float>();
			c->nearPlane = node_9738469372465["near"].as<float>();
			c->farPlane = node_9738469372465["far"].as<float>();
			c->clearColor = node_9738469372465["clearColor"].as<glm::vec3>();
			// (*_iar) >> c->fov;
			// (*_iar) >> c->nearPlane;
			// (*_iar) >> c->farPlane;
			break;
		case ser_mode::write_mode:
			node_9738469372465["fov"] = c->fov;
			node_9738469372465["near"] = c->nearPlane;
			node_9738469372465["far"] = c->farPlane;
			node_9738469372465["clearColor"] = c->clearColor;
			// (*_oar) << c->fov;
			// (*_oar) << c->nearPlane;
			// (*_oar) << c->farPlane;
			break;
		default:
			cout << "no mode provided";
			break;
		}
	}

private:
};

extern std::map<int, std::function<void(camera &)>> renderShit;