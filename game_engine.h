
#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <iostream>
#include <assimp/postprocess.h>
#include <chrono>
#include "include.h"
#include "Model.h"
#include "Shader.h"
#include <thread>

#include "gpu_vector.h"
#include "Input.h"
#include <atomic>
//#include "Component.h"
#include "game_object.h"
#include "rendering.h"
#include "game_engine_components.h"
#include "physics.h"
#include "particles.h"
#include "BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h"
#include "bullet/btBulletDynamicsCommon.h"
using namespace glm;
using namespace std;

enum update_type
{
	update,
	lateupdate
};
struct updateJob
{
	componentStorageBase *componentStorage;
	int size;
	update_type ut;
	barrier *_barrier;
	updateJob() {}
	updateJob(componentStorageBase *csb, update_type _ut, int _size, barrier *b) : componentStorage(csb), size(_size), ut(_ut), _barrier(b) {}
};

GLFWwindow *window;
GLdouble lastFrame = 0;
Shader *shadowShader;
Shader *OmniShadowShader;

bool hideMouse = true;
atomic<bool> renderDone(false);
atomic<bool> renderThreadReady(false);
game_object *player;
glm::mat4 proj;
thread *renderThread;
bool recieveMouse = true;

vector<mutex> updateLocks(concurrency::numThreads);
vector<queue<updateJob>> updateWork(concurrency::numThreads);

componentStorageBase *copyWorkers;

/////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////GL WINDOW FUNCTIONS////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

void window_close_callback(GLFWwindow *window)
{
	//if (!time_to_close)
	glfwSetWindowShouldClose(window, GLFW_TRUE);
}
void mouseFrameBegin()
{
	Input.Mouse.xOffset = Input.Mouse.yOffset = Input.Mouse.mouseScroll = 0;
}
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	if (button >= 0 && button < 1024)
	{
		if (action == GLFW_PRESS)
		{
			Input.Mouse.mouseButtons[button] = true;
		}
		else if (action == GLFW_RELEASE)
		{
			Input.Mouse.mouseButtons[button] = false;
		}
	}
}

void MouseCallback(GLFWwindow *window, double xPos, double yPos)
{
	if (recieveMouse)
	{
		if (Input.Mouse.firstMouse)
		{
			Input.Mouse.lastX = xPos;
			Input.Mouse.lastY = yPos;
			Input.Mouse.firstMouse = false;
		}

		Input.Mouse.xOffset = xPos - Input.Mouse.lastX;
		Input.Mouse.yOffset = Input.Mouse.lastY - yPos;

		Input.Mouse.lastX = xPos;
		Input.Mouse.lastY = yPos;
	}
	//camera.ProcessMouseMovement(xOffset, yOffset);
}
void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mode)
{

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
		{
			Input.keys[key] = true;
			Input.keyDowns[key] = true;
		}
		else if (action == GLFW_RELEASE)
		{
			Input.keys[key] = false;
		}
	}
}
void ScrollCallback(GLFWwindow *window, double xOffset, double yOffset)
{
	Input.Mouse.mouseScroll = yOffset;
	//camera.ProcessMouseScroll(yOffset);
}
void window_size_callback(GLFWwindow *window, int width, int height)
{
	glfwSetWindowSize(window, width, height);
}
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
	SCREEN_WIDTH = width;
	SCREEN_HEIGHT = height;
	glViewport(0, 0, width, height);
}
bool eventsPollDone;
void updateInfo()
{
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
	eventsPollDone = true;
}
void cam_render(glm::mat4 rot, glm::mat4 proj, glm::mat4 view);

glm::mat4 getProjection()
{
	return glm::perspective(glm::radians(60.f), (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, (GLfloat).1f, (GLfloat)1e32f);
}

mutex gpuDataLock;
int frameCounter = 0;
void renderThreadFunc()
{

	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_DOUBLEBUFFER, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

	window = glfwCreateWindow(WIDTH, HEIGHT, "game engine", nullptr, nullptr);

	glfwGetFramebufferSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);
	if (hideMouse)
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwSetWindowSizeCallback(window, window_size_callback);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetCursorPosCallback(window, MouseCallback);
	glfwSetScrollCallback(window, ScrollCallback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetWindowCloseCallback(window, window_close_callback);

	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (window == nullptr)
	{
		cout << "failed to create GLFW window" << endl;
		glfwTerminate();

		throw EXIT_FAILURE;
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);
	//glewExperimental = GL_TRUE;

	if (glewInit() != GLEW_OK)
	{
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

	Shader matProgram("res/shaders/mat.comp");

	GLint max_buffers;
	glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &max_buffers);
	cout << max_buffers << endl;

	// shadowShader = new Shader("res/shaders/directional_shadow_map.vert", "res/shaders/directional_shadow_map.frag", false);
	// OmniShadowShader = new Shader("res/shaders/omni_shadow_map.vert", "res/shaders/omni_shadow_map.geom", "res/shaders/omni_shadow_map.frag", false);

	renderThreadReady.exchange(true);
	GPU_TRANSFORMS = new gpu_vector<_transform>();
	GPU_TRANSFORMS->ownStorage();
	// GPU_TRANSFORMS->storage = &TRANSFORMS.data;
	// log("here");
	// ids->init();

	initParticles();
	particle_renderer.init();

	timer stopWatch;

	renderDone.store(true);
	// ::proj = glm::perspective(glm::radians(60.f), (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, 1.f, 1e10f);
	while (true)
	{
		// log("render loop");
		// log(to_string(renderWork.size()));
		if (renderWork.size() > 0)
		{
			renderLock.lock();
			renderJob rj = renderWork.front();
			renderWork.pop();
			switch (rj.type)
			{
			case doFunc: //    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				renderLock.unlock();

				rj.work();
				break;
			case renderNum::render:
			{
				gpuTimer gt;

				stopWatch.start();
				updateInfo();

				//////////////////////////////////////////////////////////////////////////////
				/////////////////////////////// Lock Transform ///////////////////////////////
				//////////////////////////////////////////////////////////////////////////////
				auto cameras = ((componentStorage<_camera>*)allcomponents.at(typeid(_camera).hash_code()));
				gpuDataLock.lock();

				gt.start();
				GPU_TRANSFORMS->bufferData();
				appendStat("transforms buffer", gt.stop());

				uint emitterInitCount = emitterInits.size();
				gpu_emitter_inits->bufferData(emitterInits);
				glFlush();
				

				// glm::mat4 view;
				// glm::mat4 rot;
				// glm::mat4 proj;
				for(_camera& c : cameras->data.data){
					c.view = c.GetViewMatrix();
					c.rot = c.getRotationMatrix();
					c.proj = c.getProjection();
				}

				_camera::initPrepRender(matProgram);
				gpuDataLock.unlock();

				//////////////////////////////////////////////////////////////////////////////
				////////////////////////////// Unlock Transform //////////////////////////////
				//////////////////////////////////////////////////////////////////////////////

				gt.start();
				updateParticles(mainCamPos,emitterInitCount); // change orient to camera in sort particles
				appendStat("particles compute", gt.stop());

				// cam_render(rj.rot, rj.proj, rj.view);
					glClearColor(0.6f, 0.7f, 1.f, 1.0f);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				
				for(_camera& c : cameras->data.data){
					// render geometry
					gt.start();
					c.prepRender(matProgram);
					appendStat("matrix compute", gt.stop());

					glEnable(GL_CULL_FACE);
					glDepthMask(GL_TRUE);
					gt.start();
					c.render();
					appendStat("render cam", gt.stop());

					// sort particles
					timer t;
					t.start();
					particle_renderer.sortParticles(c.proj * c.rot * c.view, c.rot * c.view, mainCamPos);
					appendStat("particles sort", t.stop());
					
					// render particles
					gt.start();
					// glDisable(GL_CULL_FACE);
					// glDepthMask(GL_FALSE);
					particle_renderer.drawParticles(c.view, c.rot, c.proj);
					appendStat("render particles", gt.stop());
				}


				glDepthMask(GL_TRUE);
				glfwSwapBuffers(window);
				glFlush();
				appendStat("render", stopWatch.stop());
				//renderDone.store(true);
			}
				renderLock.unlock();
				break;
			case rquit:
				particle_renderer.end();
				while (gpu_buffers.size() > 0)
				{
					(gpu_buffers.begin()->second)->deleteBuffer();
				}

				glFlush();
				renderThreadReady.exchange(false);
				glfwTerminate();

				renderLock.unlock();
				return;
			default:
				break;
			}
		}
		// else
		// {
			this_thread::sleep_for(1ns);
		// }
	}
}

// scripting
mutex antiBullshitDevice;

void lockUpdate()
{
	for (auto &i : updateLocks)
		i.lock();
}
void unlockUpdate()
{
	for (auto &i : updateLocks)
		i.unlock();
}
double cleanupTime = 0;
void cleanup()
{
	timer t;
	t.start();
	gameLock.lock();
	for (auto &i : toRemove)
	{
		i->erase();
	}
	toRemove.clear();

	for (auto &i : toDestroy)
	{
		i->gameObject->_destroy();
		i->_destroy();
	}
	toDestroy.clear();
	gameLock.unlock();
	cleanupTime += t.stop();
}

vector<size_t> threadCounters = vector<size_t>(concurrency::numThreads, 0);

atomic<bool> makeOctree(false);
// barrier _barrier;

void componentUpdateThread(int index)
{
	while (true)
	{
		updateLocks[index].lock();
		if (updateWork[index].size() != 0)
		{
			updateJob uj = updateWork[index].front();
			if (uj.componentStorage == 0)
				break;
			if (uj.ut == update_type::update)
			{
				ComponentsUpdate(uj.componentStorage, index, uj.size);
			}
			else if (uj.ut == update_type::lateupdate)
			{
				ComponentsLateUpdate(uj.componentStorage, index, uj.size);
			}
			updateWork[index].pop();
		}
		updateLocks[index].unlock();
		this_thread::sleep_for(1ns);
	}
}
void init()
{
	renderThreadReady.exchange(false);
	renderThread = new thread(renderThreadFunc);

	while (!renderThreadReady.load())
		this_thread::sleep_for(1ms);

	root = new Transform(0);
	rootGameObject = new game_object(root);
	for (int i = 0; i < concurrency::numThreads; i++)
	{
		rootGameObject->addComponent<copyBuffers>();
	}
	copyWorkers = allcomponents[typeid(copyBuffers).hash_code()];
	gameEngineComponents.erase(copyWorkers);
	rootGameObject->addComponent<cullObjects>();
	gameEngineComponents.erase(allcomponents[typeid(cullObjects).hash_code()]);
}
void syncThreads()
{
	return;
	bool isSynced = false;
	while (!isSynced)
	{
		isSynced = true;
		size_t currCount = threadCounters[0];
		for (int i = 1; i < concurrency::numThreads; ++i)
		{
			if (threadCounters[i] != currCount)
			{
				isSynced = false;
				break;
			}
		}
		this_thread::sleep_for(1ns);
	}
}
void doWork(componentStorageBase* cs,update_type type){
	int s = cs->size();
	timer stopWatch;
	stopWatch.start();
	lockUpdate();
	for (int i = 0; i < concurrency::numThreads; ++i)
		updateWork[i].push(updateJob(cs, type, s, 0));
	unlockUpdate();
	this_thread::sleep_for(1ns);
	lockUpdate();
	unlockUpdate();
	if (type == update_type::update)
		appendStat(cs->name + "--update", stopWatch.stop());
	else if (type == update_type::lateupdate)
		appendStat(cs->name + "--late_update", stopWatch.stop());
}
void doLoopIteration(set<componentStorageBase *> &ssb, bool doCleanUp = true)
{
	// UPDATE
	for (auto &j : ssb)
	{
		componentStorageBase *cb = j;
		int s = cb->size();
		doWork(j,update_type::update);
		if (doCleanUp)
			cleanup();
	}
	// LATE UPDATE
	for (auto &j : ssb)
	{
		componentStorageBase *cb = j;
		int s = cb->size();

		doWork(j,update_type::lateupdate);
		if (doCleanUp)
			cleanup();
	}

}


void waitForWork(){
	
		bool working = true;
		while (working)
		{
			working = false;
			lockUpdate();
			for (auto i : updateWork)
			{
				if (i.size() > 0)
				{
					working = true;
					break;
				}
			}
			unlockUpdate();
			this_thread::sleep_for(1ns);
		}
}

float maxGameDuration = INFINITY;
void run(btDynamicsWorld* World)
{
	timer stopWatch;

	vector<thread *> workers;
	for (int i = 0; i < concurrency::numThreads; i++)
		workers.push_back(new thread(componentUpdateThread, i));

	componentStorageBase *colliders;
	for (auto &j : allcomponents)
	{
		if (j.first == typeid(copyBuffers).hash_code())
		{
			copyWorkers = j.second;
			continue;
		}
		if (j.first == typeid(collider).hash_code())
		{
			colliders = j.second;
			continue;
		}
	}
	eventsPollDone = true;
	unlockUpdate();

	for(auto & i : collisionGraph)
		collisionLayers[i.first].clear();
	// Octree->clear();

	while (!glfwWindowShouldClose(window) && Time.time < maxGameDuration)
	{

		timer gameLoopTotal;
		timer gameLoopMain;
		gameLoopTotal.start();
		cleanupTime = 0;
		while (!eventsPollDone)
		{
			this_thread::sleep_for(1ns);
		}
		eventsPollDone = false;

		gameLoopMain.start();

		// scripting
		doLoopIteration(gameComponents);

		lockUpdate();
		for(auto & i : collisionGraph)
			collisionLayers[i.first].clear();
		// Octree->clear();
		cleanup();
		unlockUpdate();
	World->stepSimulation(Time.deltaTime,1,1/30);
		waitForWork();
		cleanup();
		doLoopIteration(gameEngineComponents, false);


		waitForWork();
		lockUpdate();
		cleanup();
		appendStat("game loop main",gameLoopMain.stop());

		stopWatch.start();
		gpuDataLock.lock();
		gameLock.lock();
		appendStat("wait for data", stopWatch.stop());

		stopWatch.start();
		GPU_TRANSFORMS->storage->resize(TRANSFORMS.size());
		for (map<string, map<string, renderingMeta *>>::iterator i = renderingManager.shader_model_vector.begin(); i != renderingManager.shader_model_vector.end(); i++)
			for (map<string, renderingMeta *>::iterator j = i->second.begin(); j != i->second.end(); j++)		
				j->second->_ids->storage->resize(j->second->ids.data.size());
		

		emitterInits.clear();
		for (auto &i : emitter_inits)
			emitterInits.push_back(i.second);
		emitter_inits.clear();
		appendStat("prepare memory", stopWatch.stop());

		for (int i = 0; i < concurrency::numThreads; ++i)
			updateWork[i].push(updateJob(copyWorkers, update_type::update, concurrency::numThreads, 0));
		unlockUpdate();

		lockUpdate();
		for (int i = 0; i < concurrency::numThreads; ++i)
			updateWork[i].push(updateJob(allcomponents[typeid(cullObjects).hash_code()], update_type::update, 1, 0));
		unlockUpdate();

		// this_thread::sleep_for(2ns);

		waitForWork();
		// syncThreads();
		gameLock.unlock();
		gpuDataLock.unlock();

		// rendering
		//        stopWatch.start();
		renderJob rj;
		rj.work = [&] { return; };
		rj.type = renderNum::render;
		rj.proj = ::proj;																			//((_camera*)(cameras->front()))->getProjection();
		rj.rot = glm::lookAt(vec3(0, 0, 0), player->transform->forward(), player->transform->up()); //glm::toMat4(glm::inverse(player->transform->getRotation()));// ((_camera*)(cameras->front()))->getRotationMatrix();
		rj.view = glm::translate(-player->transform->getPosition());								// ((_camera*)(cameras->front()))->GetViewMatrix();

		renderDone.store(false);
		stopWatch.start();
		renderLock.lock();
		appendStat("wait for render",stopWatch.stop());
		renderWork.push(rj);
		renderLock.unlock();
		appendStat("clean up", cleanupTime);
		appendStat("game loop total",gameLoopTotal.stop());
		//		appendStat("rendering", stopWatch.stop());

		//        unlockUpdate();
	}

	log("end of program");
	renderJob rj;
	rj.type = rquit;

	renderLock.lock();
	renderWork.push(rj);
	renderLock.unlock();

	for (int i = 0; i < concurrency::numThreads; i++)
	{
		updateWork[i].push(updateJob(0, update_type::update, 0, 0));
		workers[i]->join();
	}
	// endBarrier.wait();
	// for (int i = 0; i < concurrency::numThreads; i++)
	// {
	// }

	while (renderThreadReady.load())
		this_thread::sleep_for(1ms);
	renderThread->join();

	cout << endl;
	componentStats.erase("");
	for (map<string, rolling_buffer>::iterator i = componentStats.begin(); i != componentStats.end(); ++i)
	{

		cout << i->first << " : " << i->second.getAverageValue() << endl;
	}
	cout << "fps : " << 1.f / Time.unscaledSmoothDeltaTime << endl;
}

void cam_render(glm::mat4 rot, glm::mat4 proj, glm::mat4 view)
{

	float farplane = 1e32f;
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glCullFace(GL_BACK);
	for (map<string, map<string, renderingMeta *>>::iterator i = renderingManager.shader_model_vector.begin(); i != renderingManager.shader_model_vector.end(); i++)
	{
		Shader *currShader = i->second.begin()->second->s.s->shader;
		currShader->Use();
		for (map<string, renderingMeta *>::iterator j = i->second.begin(); j != i->second.end(); j++)
		{
			glUseProgram(currShader->Program);
			glUniform1f(glGetUniformLocation(currShader->Program, "material.shininess"), 32);
			glUniform1f(glGetUniformLocation(currShader->Program, "FC"), 2.0 / log2(farplane + 1));
			glUniform3fv(glGetUniformLocation(currShader->Program, "viewPos"), 1, glm::value_ptr(mainCamPos));
			glUniform1f(glGetUniformLocation(currShader->Program, "screenHeight"), (float)SCREEN_HEIGHT);
			glUniform1f(glGetUniformLocation(currShader->Program, "screenWidth"), (float)SCREEN_WIDTH);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, j->second->_ids->bufferId);

			j->second->m.m->model->Draw(*currShader, j->second->ids.size());
		}
	}
}
