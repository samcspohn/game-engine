
#pragma once
#include "renderthread.h"
#include <omp.h>
// #include "tbb/task_scheduler_init.h"
// #include <GL/glew.h>
// #include <GLFW/glfw3.h>
// #include <glm/glm.hpp>
// #include <glm/gtc/matrix_transform.hpp>
// #include <glm/gtc/type_ptr.hpp>
// #include <glm/gtx/quaternion.hpp>
// #include <glm/gtx/rotate_vector.hpp>
// #include <iostream>
// #include <assimp/postprocess.h>
// #include <chrono>
// #include "Model.h"
// #include "Shader.h"
// #include <thread>

// #include "gpu_vector.h"
// #include "Input.h"
// #include <atomic>
// #include "game_object.h"
// #include "rendering.h"
// #include "game_engine_components.h"
// #include "physics.h"
// #include "particles.h"
// #include "physics_.h"
// #include "audio.h"
// #include "gui.h"

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
	updateJob() {}
	updateJob(componentStorageBase *csb, update_type _ut, int _size) : componentStorage(csb), size(_size), ut(_ut) {}
};

atomic<int> numCubes(0);
game_object *rootGameObject;
glm::mat4 proj;
thread *renderThread;
// bool recieveMouse = true;

vector<mutex> updateLocks(concurrency::numThreads);
vector<queue<updateJob>> updateWork(concurrency::numThreads);

componentStorageBase *copyWorkers;
float maxGameDuration = INFINITY;

void doLoopIteration(set<componentStorageBase *> &ssb, bool doCleanUp = true)
{
	timer stopWatch;

	// //UPDATE
	for (auto &j : ssb)
	{
		componentStorageBase *cb = j;
		if(cb->hasUpdate()){
			stopWatch.start();
			// int size = cb->size();
			// #pragma omp parallel
			// {
				// int id = omp_get_thread_num();
				cb->update();
				// int size = cb->size();
				// #pragma omp parallel for
				// for(int i = 0; i < size; ++i){
				// 	if(cb->getv(i)){
				// 		cb->get(i)->update();
				// 	}
				// }
			// }
			float dur = stopWatch.stop();
			appendStat(cb->name + "--update", dur);
		}
	}
	// LATE //UPDATE
	for (auto &j : ssb)
	{
		componentStorageBase *cb = j;
		if(cb->hasLateUpdate()){
			stopWatch.start();
			// int size = cb->size();
			// #pragma omp parallel
			// {
				// int id = omp_get_thread_num();
				cb->lateUpdate();
			// }
			appendStat(cb->name + "--late_update", stopWatch.stop());
		}
	}

}
// void waitForWork(){
// 	bool working = true;
// 	while (working)
// 	{
// 		working = false;
// 		lockUpdate();
// 		for (auto i : updateWork)
// 		{
// 			if (i.size() > 0)
// 			{
// 				working = true;
// 				break;
// 			}
// 		}
// 		unlockUpdate();
// 		this_thread::sleep_for(1ns);
// 	}
// }

void init()
{
	// tbb::task_scheduler_init init;

	audioManager::init();
	renderThreadReady.exchange(false);
	renderThread = new thread(renderThreadFunc);
	pm = new _physicsManager();

	while (!renderThreadReady.load())
		this_thread::sleep_for(1ms);

	root = new Transform(0);
	rootGameObject = new game_object(root);
	for (int i = 0; i < concurrency::numThreads; i++)
	{
		rootGameObject->addComponent<copyBuffers>()->id = i;
	}
	copyWorkers = allcomponents[typeid(copyBuffers).hash_code()];
	transformIdThreadcache = vector<vector<GLuint>>(copyWorkers->size());
	transformThreadcache = vector<vector<_transform>>(copyWorkers->size());
	gameEngineComponents.erase(copyWorkers);
}

void run()
{
	timer stopWatch;
	copyWorkers = COMPONENT_LIST(copyBuffers);
	eventsPollDone = true;
	// unlockUpdate();

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

		for(auto & i : collisionGraph)
			collisionLayers[i.first].clear();

		doLoopIteration(gameEngineComponents, false);

		stopWatch.start();
		pm->simulate(Time.deltaTime);
		appendStat("physics simulation",stopWatch.stop());

		appendStat("game loop main",gameLoopMain.stop());

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
				c.camInv = glm::mat3(c.rot);
				c.cullpos = c.pos;
			}
		}
		////////////////////////////////////// set up transforms/renderer data to buffer //////////////////////////////////////
		stopWatch.start();
		for (map<string, map<string, renderingMeta *>>::iterator i = renderingManager::shader_model_vector.begin(); i != renderingManager::shader_model_vector.end(); i++)
			for (map<string, renderingMeta *>::iterator j = i->second.begin(); j != i->second.end(); j++)		
				j->second->_transformIds->storage->resize(j->second->ids.size());
		

		////////////////////////////////////// copy transforms/renderer data to buffer //////////////////////////////////////
		copyWorkers->update();
		appendStat("copy buffers", stopWatch.stop());

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
		// waitForWork();
		////////////////////////////////////// switch particle burst buffer //////////////////////////////////////
		swapBurstBuffer();

		////////////////////////////////////// cull objects //////////////////////////////////////
		if(Input.getKeyDown(GLFW_KEY_B)){
			cameras->data.data.front().lockFrustum = !cameras->data.data.front().lockFrustum;
		}

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

	// for (int i = 0; i < concurrency::numThreads; i++)
	// {
	// 	updateWork[i].push(updateJob(0, update_type::update, 0));
	// 	workers[i]->join();
	// 	delete workers[i];
	// }
	while (renderThreadReady.load())
		this_thread::sleep_for(1ms);
	renderThread->join();

	rootGameObject->destroy();
	destroyAllComponents();
	destroyRendering();
	audioManager::destroy();
	pm->destroy();
	delete pm;
	cout << endl;
	componentStats.erase("");
	for (map<string, rolling_buffer>::iterator i = componentStats.begin(); i != componentStats.end(); ++i)
	{
		cout << i->first << " -- avg: " << i->second.getAverageValue() << " -- stdDev: " << i->second.getStdDeviation() << endl;
	}
	cout << "fps : " << 1.f / Time.unscaledSmoothDeltaTime << endl;

}
