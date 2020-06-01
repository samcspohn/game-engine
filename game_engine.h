
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
#include "game_object.h"
#include "rendering.h"
#include "game_engine_components.h"
#include "physics.h"
#include "particles.h"
#include "BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h"
#include "bullet/btBulletDynamicsCommon.h"
#include "bullet/BulletDynamics/Dynamics/btDiscreteDynamicsWorldMt.h"
#include "physics_.h"

#include "gui.h"

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
	Barrier *_barrier;
	updateJob() {}
	updateJob(componentStorageBase *csb, update_type _ut, int _size, Barrier *b) : componentStorage(csb), size(_size), ut(_ut), _barrier(b) {}
};

atomic<int> numCubes(0);

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
float maxGameDuration = INFINITY;

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
void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
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
void updateTiming()
{
	Input.resetKeyDowns();
	mouseFrameBegin();
	glfwPollEvents();

	double currentFrame = glfwGetTime();
	Time.unscaledTime = currentFrame;
	Time.unscaledDeltaTime = currentFrame - lastFrame;
	Time.deltaTime = _min(Time.unscaledDeltaTime, 1.f / 10.f) * Time.timeScale;
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

struct renderData{
	glm::mat4 vp; glm::mat4 view; glm::vec3 camPos; glm::vec2 screen; glm::vec3 cullPos; glm::mat3 camInv; glm::mat4 rot; glm::mat4 proj;
};
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
	if (window == nullptr)
	{
		cout << "failed to create GLFW window" << endl;
		glfwTerminate();

		throw EXIT_FAILURE;
	}

/////////////////////////////////////////////////////////////
///////////////////////// GUI ////////////////////////////////
/////////////////////////////////////////////////////////////

	IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls



    // Setup style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();
    // ImGui::StyleColorsLight();

	auto font_default = io.Fonts->AddFontDefault();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

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

	GPU_TRANSFORMS = new gpu_vector<_transform>();
	GPU_TRANSFORMS->ownStorage();

	initParticles();
	particle_renderer.init();

	timer stopWatch;

	renderDone.store(true);
	renderThreadReady.exchange(true);
	while (true)
	{
		// log("render loop");
		// log(to_string(renderWork.size()));
		if (renderWork.size() > 0)
		{
			renderLock.lock();
			renderJob* rj = renderWork.front();
			renderWork.pop();
			switch (rj->type)
			{
			case doFunc: //    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				renderLock.unlock();

				rj->work();

				if(rj->completed == 0){
					rj->completed = 1;
					while(rj->completed != 2){
						this_thread::sleep_for(1ns);
					}
				}
				
				break;
			case renderNum::render:
			{
				gpuTimer gt;

				stopWatch.start();
				updateTiming();


				auto cameras = ((componentStorage<_camera>*)allcomponents.at(typeid(_camera).hash_code()));

				gt.start();
				GPU_TRANSFORMS->bufferData();
				appendStat("transforms buffer", gt.stop());

				uint emitterInitCount = emitterInits.size();
				gpu_emitter_inits->bufferData(emitterInits);
				gpu_emitter_prototypes->bufferData();
				gpu_particle_bursts->bufferData();
				glFlush();
				
				_camera::initPrepRender(matProgram);


				gt.start();
				updateParticles(mainCamPos,emitterInitCount); // change orient to camera in sort particles
				appendStat("particles compute", gt.stop());
				vector<renderData> r_d;
				for(_camera& c : cameras->data.data){
					gt.start();
					c.prepRender(matProgram);
					appendStat("matrix compute", gt.stop());
					r_d.push_back(renderData());
					r_d.back().vp = c.proj * c.rot * c.view;
					r_d.back().rot = c.rot;
					r_d.back().view = c.view;
					r_d.back().camPos = c.pos;
					r_d.back().screen = c.screen;
					r_d.back().cullPos = c.cullpos;
					r_d.back().camInv = c.camInv;
					r_d.back().proj = c.proj;


					// sort particles
					timer t;
					t.start();
					if(!c.lockFrustum)
						particle_renderer.setCamCull(c.camInv,c.cullpos);
					particle_renderer.sortParticles(c.proj * c.rot * c.view, c.rot * c.view, mainCamPos,c.screen);
					appendStat("particles sort", t.stop());
				}
				renderLock.unlock();

				// cam_render(rj.rot, rj.proj, rj.view);
				glClearColor(0.6f, 0.7f, 1.f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				
				int k = 0;
				for(_camera& c : cameras->data.data){
					glEnable(GL_CULL_FACE);
					glDepthMask(GL_TRUE);
					gt.start();
					c.render();
					appendStat("render cam", gt.stop());

					
					// render particle
					gt.start();
					glDisable(GL_CULL_FACE);
					glDepthMask(GL_FALSE);
					particle_renderer.drawParticles(r_d[k].view, r_d[k].rot, r_d[k].proj);
					appendStat("render particles", gt.stop());
					glDepthMask(GL_TRUE);
				}

/////////////////////////////////////////////////////////////
///////////////////////// GUI ////////////////////////////////
/////////////////////////////////////////////////////////////
IM_ASSERT(ImGui::GetCurrentContext() != NULL && "Missing dear imgui context. Refer to examples app!"); // Exceptionally add an extra assert here for people confused with initial dear imgui setup
				ImGui_ImplOpenGL3_NewFrame();
				ImGui_ImplGlfw_NewFrame();
				ImGui::NewFrame();

				ImGui::PushFont(font_default);

				for(auto& i : gui::gui_windows){
					i->render();
				}
				ImGui::PopFont();
				// Rendering
				ImGui::Render();
				ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
/////////////////////////////////////////////////////////////
///////////////////////// GUI ////////////////////////////////
/////////////////////////////////////////////////////////////

				glfwSwapBuffers(window);
				glFlush();
				appendStat("render", stopWatch.stop());
				//renderDone.store(true);
			}
				break;
			case rquit:
				particle_renderer.end();
				while (gpu_buffers.size() > 0)
				{
					(gpu_buffers.begin()->second)->deleteBuffer();
				}

			// Cleanup
				ImGui_ImplOpenGL3_Shutdown();
				ImGui_ImplGlfw_Shutdown();
				ImGui::DestroyContext();
				
				glFlush();
				glfwTerminate();
				renderThreadReady.exchange(false);

				renderLock.unlock();
				delete rj;

				return;
			default:
				break;
			}
			delete rj;
		}
		// else
		// {
			this_thread::sleep_for(1ns);
		// }
	}
}

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

void componentUpdateThread(int id)
{
	while (true)
	{
		updateLocks[id].lock();
		if (updateWork[id].size() != 0)
		{
			updateJob uj = updateWork[id].front();
			if (uj.componentStorage == 0)
				break;
			if (uj.ut == update_type::update)
			{
				ComponentsUpdate(uj.componentStorage, id, uj.size);
			}
			else if (uj.ut == update_type::lateupdate)
			{
				ComponentsLateUpdate(uj.componentStorage, id, uj.size);
			}
			updateWork[id].pop();
		}
		updateLocks[id].unlock();
		this_thread::sleep_for(1ns);
	}
}
_physicsManager* pm;
void init()
{
	renderThreadReady.exchange(false);
	renderThread = new thread(renderThreadFunc);
	pm = new _physicsManager();

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
		if(cb->hasUpdate()){
			int s = cb->size();
			doWork(j,update_type::update);
		}
	}
	// LATE UPDATE
	for (auto &j : ssb)
	{
		componentStorageBase *cb = j;
		if(cb->hasLateUpdate()){
			int s = cb->size();
			doWork(j,update_type::lateupdate);
		}
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

void run()
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

		unlockUpdate();
		waitForWork();
		doLoopIteration(gameEngineComponents, false);

		pm->simulate(Time.deltaTime);
		

		waitForWork();
		lockUpdate();
		appendStat("game loop main",gameLoopMain.stop());

		//////////////////////////////////////////////////////////////////////////////
		/////////////////////////////// Lock Transform ///////////////////////////////
		//////////////////////////////////////////////////////////////////////////////

		stopWatch.start();
		renderLock.lock();
		appendStat("wait for render",stopWatch.stop());

		////////////////////////////////////// update camera data for frame ///////////////////
		auto cameras = ((componentStorage<_camera>*)allcomponents.at(typeid(_camera).hash_code()));
		
		for(_camera& c : cameras->data.data){
			c.view = c.GetViewMatrix();
			c.rot = c.getRotationMatrix();
			c.proj = c.getProjection();
			c.screen = c.getScreen();
			c.pos = c.transform->getPosition();
			if(!c.lockFrustum){
				c.camInv = glm::mat3(c.rot);// * glm::mat3(glm::translate(-camera->transform->getPosition()));
				c.cullpos = c.pos;
			}
		}
		////////////////////////////////////// set up transforms/renderer data to buffer //////////////////////////////////////
		stopWatch.start();
		GPU_TRANSFORMS->storage->resize(TRANSFORMS.size());
		for (map<string, map<string, renderingMeta *>>::iterator i = renderingManager.shader_model_vector.begin(); i != renderingManager.shader_model_vector.end(); i++)
			for (map<string, renderingMeta *>::iterator j = i->second.begin(); j != i->second.end(); j++)		
				j->second->_transformIds->storage->resize(j->second->ids.size());
		

		////////////////////////////////////// copy transforms/renderer data to buffer //////////////////////////////////////
		for (int i = 0; i < concurrency::numThreads; ++i)
			updateWork[i].push(updateJob(copyWorkers, update_type::update, concurrency::numThreads, 0));
		unlockUpdate();

		////////////////////////////////////// set up emitter init buffer //////////////////////////////////////
		timer emitterTimer;
		emitterTimer.start();
		emitterInits.clear();
		for (auto &i : emitter_inits)
			emitterInits.push_back(i.second);
		emitter_inits.clear();
		float emitTime = emitterTimer.stop();
		appendStat("copy emitter inits", emitTime);

		// copy emitter inits while copying transforms/renderers
		waitForWork();
		appendStat("copy buffers", stopWatch.stop());
		////////////////////////////////////// switch particle burst buffer //////////////////////////////////////
		gpu_particle_bursts->storage->swap(particle_bursts);
		particle_bursts.clear();

		////////////////////////////////////// cull objects //////////////////////////////////////
		if(Input.getKeyDown(GLFW_KEY_B)){
			cameras->data.data.front().lockFrustum = !cameras->data.data.front().lockFrustum;
		}

		//////////////////////////////////////////////////////////////////////////////
		/////////////////////////////// Unlock Transform ///////////////////////////////
		//////////////////////////////////////////////////////////////////////////////
		renderJob* rj = new renderJob();
		rj->work = [&] { return; };
		rj->type = renderNum::render;
		renderDone.store(false);

		renderWork.push(rj);
		renderLock.unlock();
		appendStat("game loop total",gameLoopTotal.stop());

	}

	log("end of program");
	renderJob* rj = new renderJob();
	rj->type = rquit;

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
		cout << i->first << " -- avg: " << i->second.getAverageValue() << " -- stdDev: " << i->second.getStdDeviation() << endl;
	}
	cout << "fps : " << 1.f / Time.unscaledSmoothDeltaTime << endl;
}
