
#pragma once
#include "renderthread.h"
#include <tbb/task_scheduler_observer.h>
#include <sched.h>
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
			cb->update();
			appendStat(cb->name + "--update", stopWatch.stop());
			// this_thread::sleep_for(2ns);
		}
	}
	// LATE //UPDATE
	for (auto &j : ssb)
	{
		componentStorageBase *cb = j;
		if(cb->hasLateUpdate()){
			stopWatch.start();
			cb->lateUpdate();
			appendStat(cb->name + "--late_update", stopWatch.stop());
		}
		// this_thread::sleep_for(2ns);
	}

}

// class pinning_observer: public tbb::task_scheduler_observer {
// public:
//     affinity_mask_t m_mask; // HW affinity mask to be used for threads in an arena
//     pinning_observer( tbb::task_arena &a, affinity_mask_t mask )
//       : tbb::task_scheduler_observer(a), m_mask(mask) {
//         observe(true); // activate the observer
//     }
//     /*override*/ void on_scheduler_entry( bool worker ) {
//         set_thread_affinity(tbb::this_task_arena::current_thread_index(), m_mask);
//     }
//     /*override*/ void on_scheduler_exit( bool worker ) { }
// };

void init()
{
	// tbb::task_scheduler_init init;
	concurrency::pinningObserver.observe(true);
	audioManager::init();
	renderThreadReady.exchange(false);

	renderThread = new thread(renderThreadFunc);

	while (!renderThreadReady.load())
		this_thread::sleep_for(1ms);
	waitForRenderJob([](){});

	root = new Transform(0);
	rootGameObject = new game_object(root);
	for (int i = 0; i < concurrency::numThreads; i++)
	{
		rootGameObject->addComponent<copyBuffers>()->id = i;
	}

	copyWorkers = COMPONENT_LIST(copyBuffers);
	transformIdThreadcache = vector<vector<GLuint>>(copyWorkers->size());
	// transformThreadcache = vector<vector<_transform>>(copyWorkers->size());
	gameEngineComponents.erase(copyWorkers);
}

void run()
{
	timer stopWatch;
	// eventsPollDone = true;
	for(auto & i : collisionGraph)
		collisionLayers[i.first].clear();

	timer gameLoopTotal;
	timer gameLoopMain;
	while (!glfwWindowShouldClose(window) && Time.time < maxGameDuration)
	{
		gameLoopTotal.start();
		gameLoopMain.start();
		// scripting
		doLoopIteration(gameComponents);
		for(auto & i : collisionGraph)
			collisionLayers[i.first].clear();
		doLoopIteration(gameEngineComponents, false);
		appendStat("game loop main",gameLoopMain.stop());


		waitForRenderJob([](){updateTiming();});

		stopWatch.start();
		renderLock.lock();
		appendStat("wait for render",stopWatch.stop());

		transformsBuffered.store(false);
		////////////////////////////////////// update camera data for frame ///////////////////
		auto cameras = ((componentStorage<_camera>*)allcomponents.at(typeid(_camera).hash_code()));
		
		render_data& rd = RENDER_QUEUE.push();
		for(_camera& c : cameras->data.data){
			c.calcFrame();
		}
		////////////////////////////////////// set up transforms/renderer data to buffer //////////////////////////////////////
		stopWatch.start();
		int __renderersSize = 0;
		// int __rendererOffsetsSize = 0;
		__renderer_offsets->storage->clear();
		__rendererMetas->storage->clear();

		batchManager::updateBatches();
		__renderMeta rm;
		rm.min = 0;
		rm.max = 1e32f;
		for(auto &i : batchManager::batches.back()){
			for(auto &j : i.second){
				for(auto &k : j.second){
					__renderer_offsets->storage->push_back(__renderersSize);
					rm.radius = k.first->m.m->radius;
					__rendererMetas->storage->push_back(rm);
					__renderersSize += k.first->ids.size();
				}
			}
		}
		__RENDERERS->storage->resize(__renderersSize);
		////////////////////////////////////// copy transforms/renderer data to buffer //////////////////////////////////////
		copyWorkers->update();
		int bufferSize = 0;
		for(int i = 0; i < concurrency::numThreads; i++){
			((copyBuffers*)copyWorkers->get(i))->offset = bufferSize;
			bufferSize += transformIdThreadcache[i].size();
		}
		
		rd.transformIdsToBuffer.resize(bufferSize);
		rd.transformsToBuffer.resize(bufferSize);
		copyWorkers->lateUpdate();
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
	waitForRenderJob([&](){});

	concurrency::pinningObserver.observe(false);
	rootGameObject->destroy();
	destroyAllComponents();
	audioManager::destroy();

	renderJob* rj = new renderJob();
	rj->type = rquit;
	renderLock.lock();
	renderWork.push(rj);
	renderLock.unlock();
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
