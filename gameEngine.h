#pragma once

#include <iostream>

#include <GL\glew.h>
#include <GLFW\glfw3.h>
#include "SOIL2\SOIL2.h"

#include <glm\glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <vector>
#include <list>
#include <algorithm>
#include <memory>
#include <map>
#include <deque>
#include <tbb/tbb.h>

#include <thread>
#include <mutex>
#include <queue>
#include <set>
#include <deque>
#include<math.h>
#include <chrono>

#include "game_object.h"
#include "transform.h"
#include "component.h"
#include "gpu_vector.h"
#include "Shader.h"
#include "Model.h"
#include "helper1.h"

#include "concurrency.h"
#include "Input.h"
#include "collider.h"
#include "rendering.h"
#include "game_engine_components.h"

//#define DEBUG

using namespace std;
using namespace chrono;

GLFWwindow* window;
double lastFrame = 0.0f;

class game_object;
class _renderer;
class Transform;
class _camera;
class floatingOrigin;
class _worker;



class dataMove;
Shader* shadowShader;
Shader* OmniShadowShader;


void window_close_callback(GLFWwindow* window)
{
	//if (!time_to_close)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}
void mouseFrameBegin() {
	Input.Mouse.xOffset = Input.Mouse.yOffset = Input.Mouse.mouseScroll = 0;

}
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button >= 0 && button < 1024) {
		if (action == GLFW_PRESS) {
			Input.Mouse.mouseButtons[button] = true;
		}
		else if (action == GLFW_RELEASE) {
			Input.Mouse.mouseButtons[button] = false;
		}
	}
}

bool recieveMouse = true;
void MouseCallback(GLFWwindow *window, double xPos, double yPos) {
	if (recieveMouse) {
		if (Input.Mouse.firstMouse) {
			Input.Mouse.lastX = xPos;
			Input.Mouse.lastY = yPos;
			Input.Mouse.firstMouse = false;
		}

		Input.Mouse.xOffset = xPos - Input.Mouse.lastX;
		Input.Mouse.yOffset = Input.Mouse.lastY - yPos;

		Input.Mouse.lastX = xPos;
		Input.Mouse.lastY = yPos;
	}
}
void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mode) {

	if (key >= 0 && key < 1024) {
		if (action == GLFW_PRESS) {
			Input.keys[key] = true;
			Input.keyDowns[key] = true;
		}
		else if (action == GLFW_RELEASE) {
			Input.keys[key] = false;
		}
	}
}
void ScrollCallback(GLFWwindow *window, double xOffset, double yOffset) {
	Input.Mouse.mouseScroll = yOffset;
	//camera.ProcessMouseScroll(yOffset);
}
void window_size_callback(GLFWwindow* window, int width, int height)
{
	glfwSetWindowSize(window, width, height);
}
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	SCREEN_WIDTH = width;
	SCREEN_HEIGHT = height;
	glViewport(0, 0, width, height);

}


enum Func { update, updateData, quit }currentFunction;


glm::vec3 randomSphere() {
	glm::vec3 d = glm::normalize(glm::vec3(randf() * 2 - 1.0f, randf() * 2 - 1.0f, randf() * 2 - 1.0f));
	float c =  powf(randf(), 1.0f / 3.0f);
	return d * c;
}



namespace game_engine
{
	static void run();
	static GLuint init();


	list<dataMove*> dataMoversl;


	map<string, rolling_buffer> componentStats;

	int frameCount = 0;
};
using namespace game_engine;




void appendStat(string name, float dtime) {
	auto a = componentStats.find(name);
	if (a != componentStats.end())
		a->second.add(dtime);
	else {
		componentStats[name] = rolling_buffer(200);
		componentStats[name].add(dtime);
	}
}





glm::vec3 getCol(float d) {
	float r, g, b;
	if (d < .2) {//1
		r = 1;
		g = d / .2;
		b = 0;
	}
	else if (d >= .2 && d < .4) {//2
		r = 1 - (d - .2) / .2;
		g = 1;
		b = 0;
	}
	else if (d >= .4 && d < .6) {//3
		r = 0;
		g = 1;
		b = (d - .4) / .2;
	}
	else if (d >= .6 && d < .8) {//4
		r = 0;
		g = 1 - (d - .6) / .2;
		b = 1;
	}
	else if (d >= .8) {//5
		r = (d - .8) / .2;
		g = 0;
		b = 0;
	}
	return glm::vec3(r, g, b);
}


Shader* matShader = 0;
Model* model = 0;




void cam_render(glm::mat4 rot, glm::mat4 proj, glm::mat4 view) {

	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glCullFace(GL_BACK);
	for (map<string, map<string, renderingMeta*> >::iterator i = renderingManager.shader_model_vector.begin(); i != renderingManager.shader_model_vector.end(); i++) {
		Shader* currShader = i->second.begin()->second->s.s->shader;
		currShader->Use();
		for (map<string, renderingMeta*>::iterator j = i->second.begin(); j != i->second.end(); j++) {
			glUseProgram(currShader->Program);
			glUniform1f(glGetUniformLocation(currShader->Program, "material.shininess"), 32);
			glUniform1f(glGetUniformLocation(currShader->Program, "farPlane"), 1e32f);
			GLuint matPView = glGetUniformLocation(currShader->Program, "view");
			GLuint matvRot = glGetUniformLocation(currShader->Program, "vRot");
			GLuint matProjection = glGetUniformLocation(currShader->Program, "proj");
			glUniformMatrix4fv(matPView, 1, false, glm::value_ptr(view));
			glUniformMatrix4fv(matvRot, 1, false, glm::value_ptr(rot));
			glUniformMatrix4fv(matProjection, 1, false, glm::value_ptr(proj));


			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, j->second->_ids->bufferId);

			j->second->m.m->model->Draw(*currShader, j->second->ids.size());
		}
	}
}

	//draw opengl stuff


atomic<bool> renderDone;
atomic<bool> inputReady;
atomic<bool> renderThreadReady;


glm::mat4 proj;

game_object* player;


void renderThreadFunc() {
	glfwInit();
	glfwSwapInterval(0);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_DOUBLEBUFFER, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

	window = glfwCreateWindow(WIDTH, HEIGHT, "game engine", nullptr, nullptr);

	glfwGetFramebufferSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwSetWindowSizeCallback(window, window_size_callback);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetCursorPosCallback(window, MouseCallback);
	glfwSetScrollCallback(window, ScrollCallback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetWindowCloseCallback(window, window_close_callback);

	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (window == nullptr) {
		cout << "failed to create GLFW window" << endl;
		glfwTerminate();

		throw EXIT_FAILURE;
	}

	glfwMakeContextCurrent(window);
	//glewExperimental = GL_TRUE;

	if (glewInit() != GLEW_OK) {
		cout << "failed to initialize GLEW" << endl;

		throw EXIT_FAILURE;
	}

	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	glEnable(GLEW_ARB_compute_shader);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_DEPTH32F_STENCIL8);
	glEnable(GL_DEPTH_CLAMP);


	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);



	GLint success;
	GLchar infoLog[512];
	GLuint matProgram = glCreateProgram();
	GLuint matComp = glCreateShader(GL_COMPUTE_SHADER);

	stringstream ss;
	ss << ifstream("res/shaders/mat.comp").rdbuf();
	string s = ss.str();
	GLchar* shaderChars = (char*)s.c_str();


	glShaderSource(matComp, 1, &shaderChars, NULL);
	glCompileShader(matComp);
	// Print compile errors if any
	glGetShaderiv(matComp, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(matComp, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	glAttachShader(matProgram, matComp);
	glLinkProgram(matProgram);

	success = 3;
	glGetProgramiv(matProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(matProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	glDeleteShader(matComp);
	GLuint matPView = glGetUniformLocation(matProgram, "view");
	GLuint matvRot = glGetUniformLocation(matProgram, "vRot");
	GLuint matProjection = glGetUniformLocation(matProgram, "projection");

	GLuint matCamPos = glGetUniformLocation(matProgram, "camPos");
	GLuint matCamForward = glGetUniformLocation(matProgram, "camForward");
	GLuint matNum = glGetUniformLocation(matProgram, "num");

	GLint max_buffers;
	glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &max_buffers);
	cout << max_buffers;


	shadowShader = new Shader("res/shaders/directional_shadow_map.vert", "res/shaders/directional_shadow_map.frag", false);
	OmniShadowShader = new Shader("res/shaders/omni_shadow_map.vert", "res/shaders/omni_shadow_map.geom", "res/shaders/omni_shadow_map.frag", false);



	renderThreadReady.exchange(true);
	GPU_RENDERERS = new gpu_vector<__renderer>();
	GPU_RENDERERS->storage = &(gpu_renderers.data);
	GPU_MATRIXES = new gpu_vector_proxy<matrix>();
	GPU_TRANSFORMS = new gpu_vector<_transform>();
	//GPU_TRANSFORMS->storage = &TRANSFORMS.data;
	GPU_TRANSFORMS->ownStorage();

	renderDone.store(true);

	proj = glm::perspective(glm::radians(60.f), (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, 1.f, 1e10f);
	while (true) {
		if (renderWork.size() > 0) {
			renderLock.lock();
			renderJob rj = renderWork.front(); renderWork.pop();
			switch (rj.type) {
			case doFunc:
				rj.work();
				renderLock.unlock();
				break;
			case renderNum::render:
			{


				//transformArray.lock();
				GPU_TRANSFORMS->bufferData();//array_heap

				GPU_RENDERERS->bufferData();
				glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
				//transformArray.unlock();
				GPU_MATRIXES->tryRealloc(GPU_MATRIXES_IDS.size());

				glUseProgram(matProgram);

				glUniformMatrix4fv(matPView, 1, GL_FALSE, glm::value_ptr(rj.view));
				glUniformMatrix4fv(matvRot, 1, GL_FALSE, glm::value_ptr(rj.rot));
				glUniformMatrix4fv(matProjection, 1, GL_FALSE, glm::value_ptr(rj.proj));

				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, GPU_TRANSFORMS->bufferId);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, GPU_RENDERERS->bufferId);
				//glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, GPU_TRANSFORM_IDS->bufferId);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, GPU_MATRIXES->bufferId);

				mainCamPos = player->transform->getPosition();
				MainCamForward = player->transform->forward();
				glUniform3f(matCamPos, mainCamPos.x, mainCamPos.y, mainCamPos.z);
				glUniform3f(matCamForward, MainCamForward.x, MainCamForward.y, MainCamForward.z);

				glUniform1ui(matNum, GPU_RENDERERS->size());

				glDispatchCompute(GPU_RENDERERS->size() / 64 + 1, 1, 1);

				glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);

				//buffer renderer ids
				for (map<string, map<string, renderingMeta*> >::iterator i = renderingManager.shader_model_vector.begin(); i != renderingManager.shader_model_vector.end(); i++) {
					for (map<string, renderingMeta*>::iterator j = i->second.begin(); j != i->second.end(); j++) {
						j->second->_ids->bufferData();
					}
				}



				glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, GPU_MATRIXES->bufferId);
				cam_render(rj.rot, rj.proj, rj.view);
				glfwSwapBuffers(window);

				renderDone.store(true);
			}
			renderLock.unlock();
			break;
			case rquit:

				delete GPU_MATRIXES;
				delete GPU_RENDERERS;
				delete GPU_TRANSFORMS;
				//delete GPU_TRANSFORM_IDS;

				for (map<string, map<string, renderingMeta*> >::iterator i = renderingManager.shader_model_vector.begin(); i != renderingManager.shader_model_vector.end(); i++) {
					for (map<string, renderingMeta*>::iterator j = i->second.begin(); j != i->second.end(); j++) {
						delete j->second->_ids;
					}
				}

				renderThreadReady.exchange(false);
				renderLock.unlock();
				return;
			default:
				break;
			}
		}
		else {
			this_thread::sleep_for(0ns);
		}

	}


}



thread* renderThread;
GLuint game_engine::init() {

	cout << "threads : " << concurrency::numThreads << endl;

	renderThreadReady.exchange(false);
	renderThread = new thread(renderThreadFunc);

	while (!renderThreadReady.load())
		this_thread::sleep_for(1ms);

	root = new Transform(0);
	rootGameObject = new game_object(root);
	for(int i = 0; i < concurrency::numThreads; i++)
		rootGameObject->addComponent<prepRenderStorage>();
	return 0;
}

void updateInfo() {
	Input.resetKeyDowns();
	mouseFrameBegin();
	glfwPollEvents();

	double currentFrame = glfwGetTime();
	Time.unscaledTime = currentFrame;
	Time.unscaledDeltaTime = currentFrame - lastFrame;
	Time.deltaTime = _min(Time.unscaledDeltaTime, 0.1f) * Time.timeScale;
	Time.time += Time.deltaTime;
	Time.timeBuffer.add(Time.unscaledDeltaTime);
	lastFrame = currentFrame;
	Time.unscaledSmoothDeltaTime = Time.timeBuffer.getAverageValue();
}


struct updateJob {
	componentStorageBase* componentStorage;
	updateJob() {}
	updateJob(componentStorageBase* csb) : componentStorage(csb) {}
};
vector<mutex> updateLocks(concurrency::numThreads);
vector<queue<updateJob> > updateWork(concurrency::numThreads);
void lockUpdate() {
	for (auto& i : updateLocks)
		i.lock();
}
void unlockUpdate() {
	for (auto& i : updateLocks)
		i.unlock();
}
void componentUpdateThread(int index) {
	while (true) {
        updateLocks[index].lock();
		if (updateWork[index].size() > 0) {
			updateJob uj = updateWork[index].front(); updateWork[index].pop();
			if (uj.componentStorage == 0)
				break;
			ComponentsUpdate(uj.componentStorage, index);
//			updateLocks[index].unlock();
		}
//		else {
//			this_thread::sleep_for(0ns);
//		}
        updateLocks[index].unlock();
	}

}
//void tbbWorkThreadfunc() {
//	tbb::parallel_for(0, (int)concurrency::numThreads, componentUpdateThread);
//}
void game_engine::run() {


	float frameRate = 0;
	Time.deltaTime = 0;
	Time.smoothDeltaTime = 0;
	Time.time = 0;
	//toDelete.clear();

	int num = concurrency::numThreads;
	thread tbbWorkThread([]() {tbb::parallel_for(0, (int)concurrency::numThreads, componentUpdateThread); });

	while (!glfwWindowShouldClose(window))
	{
		//transformArray.lock();
		for (auto& j : allcomponents) {
			if (j.first == typeid(prepRenderStorage).hash_code())
				continue;
			lockUpdate();
			for (int i = 0; i < concurrency::numThreads; ++i)
				updateWork[i].push(updateJob(j.second));
			unlockUpdate();
			this_thread::sleep_for(1ns);
		}
		//transformArray.unlock();

		//while (TRANSFORMS.density() < 0.99) { // shift
//	int loc = GO_T_refs[TRANSFORMS.size() - 1]->transform->shift();
//	if (GO_T_refs[loc]->getRenderer() != 0)
//		GO_T_refs[loc]->getRenderer()->updateTransformLoc(loc);
//}


		GPU_TRANSFORMS->storage->resize((TRANSFORMS.size() / _DEQUE_BLOCK_SIZE + 1) * _DEQUE_BLOCK_SIZE);
		lockUpdate();
		for (int i = 0; i < concurrency::numThreads; ++i)
			updateWork[i].push(updateJob(allcomponents.at(typeid(prepRenderStorage).hash_code())));
		unlockUpdate();
		this_thread::sleep_for(1ns);

		/*int itr = 0;
		for (auto& i : TRANSFORMS.data)
			GPU_TRANSFORMS->storage->at(itr++) = i;
*/
		while (!renderDone.load())
			this_thread::sleep_for(1ns);
		inputReady.exchange(false);

		renderJob rj;
		rj.type = renderNum::doFunc;
		rj.work = []() {updateInfo(); };

		renderLock.lock();
		renderWork.push(rj);
		renderLock.unlock();

		rj.type = renderNum::render;
		rj.val1 = GPU_RENDERERS->size();
		rj.proj = proj;// ((_camera*)(cameras->front()))->getProjection();
		rj.rot = glm::toMat4(glm::inverse(player->transform->getRotation()));// ((_camera*)(cameras->front()))->getRotationMatrix();
		rj.view = glm::translate(-player->transform->getPosition());// ((_camera*)(cameras->front()))->GetViewMatrix();

		renderDone.store(false);

		renderLock.lock();
		renderWork.push(rj);
		renderLock.unlock();
	}
	renderJob rj;
	rj.type = rquit;

	renderLock.lock();
	renderWork.push(rj);
	renderLock.unlock();

	while (renderThreadReady.load())
		this_thread::sleep_for(1ms);


	for (int i = 0; i < concurrency::numThreads; ++i)
		updateWork[i].push(updateJob(0));
	tbbWorkThread.join();

	glfwTerminate();


}

//_camera::_camera(GLfloat fov, GLfloat nearPlane, GLfloat farPlane)
//{
//	this->fov = fov;
//	this->nearPlane = nearPlane;
//	this->farPlane = farPlane;
//
//}


//_camera::~_camera()
//{
//}

//void _camera::render(glm::mat4 rot,glm::mat4 proj, glm::mat4 view) {
//
//	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
//
//}
