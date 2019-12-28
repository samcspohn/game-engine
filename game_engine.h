
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


using namespace glm;
using namespace std;


enum update_type{
    update,lateupdate
};
struct updateJob {
	componentStorageBase* componentStorage;
	int size;
	update_type ut;
	updateJob() {}
	updateJob(componentStorageBase* csb, update_type _ut, int _size) : componentStorage(csb), size(_size), ut(_ut) {}
};

GLFWwindow* window;
GLdouble lastFrame = 0;
Shader* shadowShader;
Shader* OmniShadowShader;

atomic<bool> renderDone(false);
atomic<bool> renderThreadReady(false);
game_object* player;
glm::mat4 proj;
thread* renderThread;
bool recieveMouse = true;
map<string, rolling_buffer> componentStats;
vector<mutex> updateLocks(concurrency::numThreads);
vector<queue<updateJob> > updateWork(concurrency::numThreads);


/////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////GL WINDOW FUNCTIONS////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////


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
	//camera.ProcessMouseMovement(xOffset, yOffset);
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
bool eventsPollDone;
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
	eventsPollDone = true;
}
void cam_render(glm::mat4 rot, glm::mat4 proj, glm::mat4 view);

glm::mat4 getProjection() {
	return glm::perspective(glm::radians(60.f), (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, (GLfloat).1f, (GLfloat)1e32f);
}


mutex gpuDataLock;

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


	Shader matProgram("res/shaders/mat.comp");


	GLint max_buffers;
	glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &max_buffers);
	cout << max_buffers << endl;


	// shadowShader = new Shader("res/shaders/directional_shadow_map.vert", "res/shaders/directional_shadow_map.frag", false);
	// OmniShadowShader = new Shader("res/shaders/omni_shadow_map.vert", "res/shaders/omni_shadow_map.geom", "res/shaders/omni_shadow_map.frag", false);



	renderThreadReady.exchange(true);
	GPU_RENDERERS = new gpu_vector<__renderer>();
	GPU_RENDERERS->ownStorage();
	GPU_MATRIXES = new gpu_vector_proxy<matrix>();
	GPU_TRANSFORMS = new gpu_vector<_transform>();
//	GPU_TRANSFORMS->ownStorage();
    GPU_TRANSFORMS->storage = &TRANSFORMS.data;
	// log("here");
	// ids->init();



	renderDone.store(true);
	proj = glm::perspective(glm::radians(60.f), (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, 1.f, 1e10f);
	while (true) {
    	// log("render loop");
		// log(to_string(renderWork.size()));
		if (renderWork.size() > 0) {
			renderLock.lock();
			renderJob rj = renderWork.front(); renderWork.pop();
			switch (rj.type) {
			case doFunc://    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

				rj.work();
				renderLock.unlock();
				break;
			case renderNum::render:
			{
                updateInfo();

				gpuDataLock.lock();
				GPU_TRANSFORMS->bufferData();//array_heap

				GPU_RENDERERS->bufferData();
				GPU_MATRIXES->tryRealloc(GPU_MATRIXES_IDS.size());

				GLuint matPView = glGetUniformLocation(matProgram.Program, "view");
				GLuint matvRot = glGetUniformLocation(matProgram.Program, "vRot");
				GLuint matProjection = glGetUniformLocation(matProgram.Program, "projection");

				GLuint matCamPos = glGetUniformLocation(matProgram.Program, "camPos");
				GLuint matCamForward = glGetUniformLocation(matProgram.Program, "camForward");
				GLuint matNum = glGetUniformLocation(matProgram.Program, "num");

				glUseProgram(matProgram.Program);

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

				gpuDataLock.unlock();

				//buffer renderer ids
				 for (map<string, map<string, renderingMeta*> >::iterator i = renderingManager.shader_model_vector.begin(); i != renderingManager.shader_model_vector.end(); i++)
				 	for (map<string, renderingMeta*>::iterator j = i->second.begin(); j != i->second.end(); j++)
				 		j->second->_ids->bufferData();
//				ids->bufferData();

				renderDone.store(true);


				glClearColor(0.6f, 0.6f, 0.6f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, GPU_MATRIXES->bufferId);
				cam_render(rj.rot, rj.proj, rj.view);
				glfwSwapBuffers(window);

				//renderDone.store(true);
			}
			renderLock.unlock();
			break;
			case rquit:

				delete GPU_MATRIXES;
				delete GPU_RENDERERS;
				delete GPU_TRANSFORMS;
//				delete GPU_TRANSFORM_IDS;

				 for (map<string, map<string, renderingMeta*> >::iterator i = renderingManager.shader_model_vector.begin(); i != renderingManager.shader_model_vector.end(); i++) {
				 	for (map<string, renderingMeta*>::iterator j = i->second.begin(); j != i->second.end(); j++) {
				 		delete j->second->_ids;
				 	}
				 }

				renderThreadReady.exchange(false);
			    glfwTerminate();

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



void appendStat(string name, float dtime) {
	auto a = componentStats.find(name);
	if (a != componentStats.end())
		a->second.add(dtime);
	else {
		componentStats[name] = rolling_buffer(200);
		componentStats[name].add(dtime);
	}
}
// scripting

void lockUpdate() {
	for (auto& i : updateLocks)
		i.lock();
}
void unlockUpdate() {
	for (auto& i : updateLocks)
		i.unlock();
}
void cleanup(){
    removeLock.lock();
    for(auto& i : toRemove){
        i->erase();
    }
    removeLock.unlock();
    destroyLock.lock();
    for(auto& i : toDestroy){
        i->gameObject->_destroy();
        i->_destroy();
    }
    destroyLock.unlock();
    toRemove.clear();
    toDestroy.clear();
}
//void componentUpdateThread(int index) {
//    timer stopWatch;
//	while (true) {
//        updateLocks[index].lock();
//		if (updateWork[index].size() > 0) {
//			updateJob uj = updateWork[index].front(); updateWork[index].pop();
//			if (uj.componentStorage == 0)
//				break;
//            if(index == 0)
//                stopWatch.start();
//			ComponentsUpdate(uj.componentStorage, index);
//			if(index == 0)
//                appendStat(uj.componentStorage->name, stopWatch.stop());
//		}
//        updateLocks[index].unlock();
//        this_thread::sleep_for(0ns);
//	}
//
//}

vector<size_t> threadCounters = vector<size_t>(concurrency::numThreads,0);

atomic<bool> makeOctree(false);

void componentUpdateThread(int index) {
    timer stopWatch;
	while (true) {
        updateLocks[index].lock();
		if (updateWork[index].size() > 0) {
			updateJob uj = updateWork[index].front(); updateWork[index].pop();
			if (uj.componentStorage == 0)
				break;
            if(index == 0)
                stopWatch.start();

            switch(uj.ut){
                case update_type::update:
                    ComponentsUpdate(uj.componentStorage, index, uj.size);
                    break;
                case update_type::lateupdate:
                    ComponentsLateUpdate(uj.componentStorage,index,uj.size);
                    break;
                default:
                    break;
            }
            threadCounters[index]++;
			if(index == 0 && uj.ut == update_type::update)
                appendStat(uj.componentStorage->name + "--update", stopWatch.stop());
            else if(index == 0 && uj.ut == update_type::lateupdate)
                appendStat(uj.componentStorage->name + "--late_update", stopWatch.stop());
		}
        updateLocks[index].unlock();
        this_thread::sleep_for(1ns);
	}
}
void init(){
    renderThreadReady.exchange(false);
	renderThread = new thread(renderThreadFunc);

	while (!renderThreadReady.load())
		this_thread::sleep_for(1ms);

    root = new Transform(0);
	rootGameObject = new game_object(root);
    for(int i = 0; i < concurrency::numThreads; i++){
        rootGameObject->addComponent<barrier>();
        rootGameObject->addComponent<copyBuffers>();
    }
}

void waitForBarrier(){
    while(barrierCounter.load() < concurrency::numThreads)
                this_thread::sleep_for(1ns);
    barrierCounter.store(0);
}

void syncThreads(){
    bool isSynced = false;
    while(!isSynced){
        isSynced = true;
        size_t currCount = threadCounters[0];
        for(int i = 1; i < concurrency::numThreads; ++i){
            if(threadCounters[i] != currCount){
                isSynced = false;
            }
        }
        this_thread::sleep_for(1ns);
    }
}
void run(){
    timer stopWatch;

    vector<thread*> workers;
    for(int i = 0; i < concurrency::numThreads; i++)
        workers.push_back(new thread(componentUpdateThread,i));

    componentStorageBase* copyWorkers;
    componentStorageBase* barriers;
    componentStorageBase* colliders;
    for (auto& j : allcomponents) {
        if (j.first == typeid(copyBuffers).hash_code()){
            copyWorkers = j.second;
            continue;
        }
        if (j.first == typeid(barrier).hash_code()){
            barriers = j.second;
            continue;
        }
        if(j.first == typeid(collider).hash_code()){
            colliders = j.second;
            continue;
        }
    }
    eventsPollDone = true;
    cleanup();

    for (int i = 0; i < concurrency::numThreads; ++i){
        updateWork[i].push(updateJob(barriers,update_type::update, concurrency::numThreads));
    }
    unlockUpdate();
    waitForBarrier();



    leaves.clear();
	octree2 = &nodes.front();
	octree2->clear();
	octree2->id = 0;
	numNodes = 1;


    while (!glfwWindowShouldClose(window))
    {

        while(!eventsPollDone){
            this_thread::sleep_for(1ns);
        }
        eventsPollDone = false;
        barrierCounter = 0;

        lockUpdate();

        syncThreads();
        size_t currCount = threadCounters[0];
        for(int i = 1; i < concurrency::numThreads; ++i){
            if(threadCounters[i] != currCount){
//                cout << "component type: " << cb->name << endl;
                cout << "threads out of sync" << endl;
                cout << "baseline: " << currCount << endl;
                cout << "offender: " << threadCounters[i] << endl;
                cout << "thread id: " << i << endl;
                throw;
            }
        }



        leaves.clear();
		octree2->clear();
		octree2 = &nodes.front();
		octree2->id = 0;
		numNodes = 1;
		nodes[0]._parent = -1;

        unlockUpdate();

        gpuDataLock.lock();
        // scripting
        // UPDATE
		for (auto& j : allcomponents) {
			if (j.first == typeid(copyBuffers).hash_code())
				continue;
            if (j.first == typeid(barriers).hash_code())
				continue;
            componentStorageBase* cb = j.second;
            int s = cb->size();

			lockUpdate();
			cleanup();
			for (int i = 0; i < concurrency::numThreads; ++i){
				updateWork[i].push(updateJob(j.second,update_type::update, s));
                updateWork[i].push(updateJob(barriers,update_type::update, concurrency::numThreads));
			}
			unlockUpdate();
			waitForBarrier();
			syncThreads();
		}
		// LATE UPDATE
        for (auto& j : allcomponents) {
			if (j.first == typeid(copyBuffers).hash_code())
				continue;
            if (j.first == typeid(barriers).hash_code())
				continue;
            componentStorageBase* cb = j.second;
            int s = cb->size();

			lockUpdate();
			cleanup();
			for (int i = 0; i < concurrency::numThreads; ++i){
				updateWork[i].push(updateJob(j.second,update_type::lateupdate, s));
                updateWork[i].push(updateJob(barriers,update_type::update, concurrency::numThreads));
			}
			unlockUpdate();
			waitForBarrier();
			syncThreads();
		}
		//barrier
        lockUpdate();
        for (int i = 0; i < concurrency::numThreads; ++i){
            updateWork[i].push(updateJob(barriers,update_type::update, concurrency::numThreads));
        }
        unlockUpdate();
        waitForBarrier();
        syncThreads();
        // wait for barrier

        lockUpdate();


        cleanup();
        while (TRANSFORMS.density() < 0.99) { // shift
            int loc = GO_T_refs[TRANSFORMS.size() - 1]->transform->shift();
            if (GO_T_refs[loc]->getRenderer() != 0)
                GO_T_refs[loc]->getRenderer()->updateTransformLoc(loc);
        }
        stopWatch.start();
        for(auto& j : allcomponents){
            if (j.first == typeid(copyBuffers).hash_code() || j.first == typeid(barriers).hash_code())
				continue;
            j.second->sort();
        }

//        GPU_TRANSFORMS->storage->resize(TRANSFORMS.size());
        GPU_RENDERERS->storage->resize(gpu_renderers.size());
        for (map<string, map<string, renderingMeta*> >::iterator i = renderingManager.shader_model_vector.begin(); i != renderingManager.shader_model_vector.end(); i++) {
            for (map<string, renderingMeta*>::iterator j = i->second.begin(); j != i->second.end(); j++) {
                j->second->_ids->storage->resize(j->second->ids.data.size());
            }
        }
        appendStat("prepare memory", stopWatch.stop());

        for (int i = 0; i < concurrency::numThreads; ++i){
            updateWork[i].push(updateJob(copyWorkers, update_type::update, concurrency::numThreads));
            updateWork[i].push(updateJob(barriers, update_type::update, concurrency::numThreads));
        }
        unlockUpdate();
        waitForBarrier();
        syncThreads();
        gpuDataLock.unlock();

		// rendering
//        stopWatch.start();
		renderJob rj;
        rj.type = renderNum::render;
		rj.val1 = GPU_RENDERERS->size();
		rj.proj = proj;//((_camera*)(cameras->front()))->getProjection();
		rj.rot = glm::lookAt(vec3(0,0,0),player->transform->forward(),player->transform->up());//glm::toMat4(glm::inverse(player->transform->getRotation()));// ((_camera*)(cameras->front()))->getRotationMatrix();
		rj.view = glm::translate(-player->transform->getPosition());// ((_camera*)(cameras->front()))->GetViewMatrix();


		renderDone.store(false);

		renderLock.lock();
		renderWork.push(rj);
		renderLock.unlock();
//		appendStat("rendering", stopWatch.stop());

//        unlockUpdate();
    }

	log("end of program");
	renderJob rj;
	rj.type = rquit;

	renderLock.lock();
	renderWork.push(rj);
	renderLock.unlock();

     for(int i = 0; i < concurrency::numThreads; i++){
        updateWork[i].push(updateJob(0,update_type::update,0));
        workers[i]->join();
     }


	while (renderThreadReady.load())
        this_thread::sleep_for(1ms);


    cout << endl;
	componentStats.erase("");
	for (map<string, rolling_buffer>::iterator i = componentStats.begin(); i != componentStats.end(); ++i) {
		cout << i->first << " : " << i->second.getAverageValue() << endl;
	}
	cout << "fps : " << 1.f / Time.unscaledSmoothDeltaTime << endl;
}


void cam_render(glm::mat4 rot, glm::mat4 proj, glm::mat4 view) {

    float farplane = 1e32f;
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glCullFace(GL_BACK);
	for (map<string, map<string, renderingMeta*> >::iterator i = renderingManager.shader_model_vector.begin(); i != renderingManager.shader_model_vector.end(); i++) {
		Shader* currShader = i->second.begin()->second->s.s->shader;
		currShader->Use();
		for (map<string, renderingMeta*>::iterator j = i->second.begin(); j != i->second.end(); j++) {
			glUseProgram(currShader->Program);
			glUniform1f(glGetUniformLocation(currShader->Program, "material.shininess"), 32);
			glUniform1f(glGetUniformLocation(currShader->Program, "FC"), 2.0 / log2(farplane + 1));

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, j->second->_ids->bufferId);

			j->second->m.m->model->Draw(*currShader, j->second->ids.size());
		}
	}
}

