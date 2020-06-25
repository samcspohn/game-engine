
#pragma once
#include "renderthread.h"
#include <omp.h>
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
// void lockUpdate()
// {
// 	for (auto &i : updateLocks)
// 		i.lock();
// }
// void unlockUpdate()
// {
// 	for (auto &i : updateLocks)
// 		i.unlock();
// }

// void componentUpdateThread(int id)
// {
// 	while (true)
// 	{
// 		updateLocks[id].lock();
// 		if (updateWork[id].size() != 0)
// 		{
// 			updateJob uj = updateWork[id].front();
// 			if (uj.componentStorage == 0)
// 				break;
// 			if (uj.ut == update_type::update)
// 			{
// 				uj.componentStorage->update(id, uj.size);
// 			}
// 			else if (uj.ut == update_type::lateupdate)
// 			{
// 				uj.componentStorage->lateUpdate(id, uj.size);
// 			}
// 			updateWork[id].pop();
// 		}
// 		updateLocks[id].unlock();
// 		this_thread::sleep_for(1ns);
// 	}
// }


// void doWork(componentStorageBase* cs,update_type type){
// 	int s = cs->size();
// 	timer stopWatch;
// 	stopWatch.start();
// 	lockUpdate();
// 	for (int i = 0; i < concurrency::numThreads; ++i)
// 		updateWork[i].push(updateJob(cs, type, s));
// 	unlockUpdate();
// 	this_thread::sleep_for(1ns);
// 	lockUpdate();
// 	unlockUpdate();
// 	if (type == update_type::update)
// 		appendStat(cs->name + "--update", stopWatch.stop());
// 	else if (type == update_type::lateupdate)
// 		appendStat(cs->name + "--late_update", stopWatch.stop());
// }
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
			// }
			appendStat(cb->name + "--update", stopWatch.stop());
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
		rootGameObject->addComponent<copyBuffers>();
	}
	copyWorkers = allcomponents[typeid(copyBuffers).hash_code()];
	transformIdThreadcache = vector<vector<GLuint>>(copyWorkers->size());
	transformThreadcache = vector<vector<_transform>>(copyWorkers->size());
	gameEngineComponents.erase(copyWorkers);
}

void run()
{
	timer stopWatch;

	// vector<thread *> workers;
	// for (int i = 0; i < concurrency::numThreads; i++)
	// 	workers.push_back(new thread(componentUpdateThread, i));

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

		// lockUpdate();
		for(auto & i : collisionGraph)
			collisionLayers[i.first].clear();
		// Octree->clear();

		// unlockUpdate();
		// waitForWork();
		doLoopIteration(gameEngineComponents, false);

		stopWatch.start();
		pm->simulate(Time.deltaTime);

		appendStat("physics simulation",stopWatch.stop());

		// waitForWork();
		// lockUpdate();
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
		for (map<string, map<string, renderingMeta *>>::iterator i = renderingManager::shader_model_vector.begin(); i != renderingManager::shader_model_vector.end(); i++)
			for (map<string, renderingMeta *>::iterator j = i->second.begin(); j != i->second.end(); j++)		
				j->second->_transformIds->storage->resize(j->second->ids.size());
		

		////////////////////////////////////// copy transforms/renderer data to buffer //////////////////////////////////////
		copyWorkers->update();
		// int size = copyWorkers->size();
		// #pragma omp parallel
		// {
		// 	int id = omp_get_thread_num();
		// 	copyWorkers->update(id, size);
		// }
		// for (int i = 0; i < concurrency::numThreads; ++i)
		// 	updateWork[i].push(updateJob(copyWorkers, update_type::update, concurrency::numThreads));
		// unlockUpdate();

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
		appendStat("copy buffers", stopWatch.stop());
		////////////////////////////////////// switch particle burst buffer //////////////////////////////////////
		swapBurstBuffer();

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
