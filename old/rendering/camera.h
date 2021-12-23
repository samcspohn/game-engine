#pragma once
#include "Component.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include "rendering.h"
#include "array_heap.h"
#include "Transform.h"
#include "helper1.h"
#include <vector>
#include "renderTexture.h"
#include "lighting.h"
#include "particles.h"

using namespace std;

// typedef glm::vec4 plane;
// struct _frustum
// {
// 	plane top;
// 	plane left;
// 	plane right;
// 	plane bottom;
// };
// struct DrawElementsIndirectCommand
// {
// 	uint count;
// 	uint primCount;
// 	uint firstIndex;
// 	uint baseVertex;
// 	uint baseInstance;
// };

void renderQuad();

// _shader billBoardGenerator;
// _shader billBoardShader;
// _model bill;

void makeBillboard(_model m, _texture t, _renderer *r);
// lightVolume lv;

struct camera{
	camera(GLfloat fov, GLfloat nearPlane, GLfloat farPlane);
	camera();
	~camera();
	
	GLfloat fov;
	GLfloat nearPlane;
	GLfloat farPlane;
	bool lockFrustum = false;
	bool inited = false;
	// _frustum f;
	// map<string, map<string, gpu_vector<GLuint>* >> shader_model_culled; // for transforms
	// map<string, map<string, gpu_vector_proxy<matrix> *>> mats;		  // for rendering
	vector<GLuint> _rendererOffsets;

	renderTexture gBuffer;
	_shader _shaderLightingPass;
	_shader _quadShader;
	_model lightVolumeModel;

	glm::mat4 view;
	glm::mat4 rot;
	glm::mat4 proj;
	glm::vec2 screen;

	glm::vec3 pos;
	glm::vec3 dir;
	glm::vec3 up;
	glm::vec3 cullpos;
	glm::mat3 camInv;

	// void onEdit();

	glm::vec2 getScreen();
	int order();
	// void onStart();
	// void onDestroy();
	void prepRender(Shader &matProgram);
	void render();
	void update(glm::vec3 position,glm::quat rotation);
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
	~_camera();
	camera* c = 0;
	// GLfloat fov;
	// GLfloat nearPlane;
	// GLfloat farPlane;
	// bool lockFrustum = false;
	// bool inited = false;
	// // _frustum f;
	// // map<string, map<string, gpu_vector<GLuint>* >> shader_model_culled; // for transforms
	// // map<string, map<string, gpu_vector_proxy<matrix> *>> mats;		  // for rendering
	// vector<GLuint> _rendererOffsets;

	// renderTexture gBuffer;
	// _shader _shaderLightingPass;
	// _shader _quadShader;
	// _model lightVolumeModel;

	// glm::mat4 view;
	// glm::mat4 rot;
	// glm::mat4 proj;
	// glm::vec2 screen;

	// glm::vec3 pos;
	// glm::vec3 dir;
	// glm::vec3 up;
	// glm::vec3 cullpos;
	// glm::mat3 camInv;

	void onEdit();

	// glm::vec2 getScreen();
	// int order();
	// void onStart();
	// void onDestroy();
	// void prepRender(Shader &matProgram);
	// void render();
	// glm::mat4 getRotationMatrix();
	// glm::mat4 GetViewMatrix();
	// glm::mat4 getProjection();
	// glm::vec3 screenPosToRay(glm::vec2 mp);
	// _frustum getFrustum();
	COPY(_camera);
	SER1(c);
private:
};
// REGISTER_COMPONENT(_camera)

extern vector<int> renderCounts;
extern vector<vector<vector<GLint>>> transformIdThreadcache;
// vector<vector<_transform>> transformThreadcache;

// vector<int> transformIdsToBuffer;
// vector<_transform> transformsToBuffer;
extern vector<vector<glm::vec3>> positionsToBuffer;
extern vector<vector<glm::quat>> rotationsToBuffer;
extern vector<vector<glm::vec3>> scalesToBuffer;
