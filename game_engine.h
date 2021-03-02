
#pragma once
#include "renderthread.h"
// #include "physics2.h"
#include "physics.h"
#include <tbb/tbb_thread.h>

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

glm::mat4 proj;
tbb::tbb_thread *renderThread;
// bool recieveMouse = true;

vector<mutex> updateLocks(concurrency::numThreads);
vector<queue<updateJob>> updateWork(concurrency::numThreads);

// componentStorageBase *copyWorkers;
float maxGameDuration = INFINITY;

void doLoopIteration(map<size_t, componentStorageBase *> &ssb, bool doCleanUp = true)
{
	timer stopWatch;

	// //UPDATE
	for (auto &j : ssb)
	{
		componentStorageBase *cb = j.second;
		if (cb->hasUpdate())
		{
			stopWatch.start();
			cb->update();
			appendStat(cb->name + "--update", stopWatch.stop());
		}
	}
	// LATE //UPDATE
	for (auto &j : ssb)
	{
		componentStorageBase *cb = j.second;
		if (cb->hasLateUpdate())
		{
			stopWatch.start();
			cb->lateUpdate();
			appendStat(cb->name + "--late_update", stopWatch.stop());
		}
	}
}

void setRootGameObject(transform2 r)
{
	rootGameObject = new game_object(root2);
}

bool guiRayCast(vec3 p, vec3 d)
{
	return raycast(p, d);
}

void init()
{

	// tbb::task_scheduler_init init;
	// concurrency::pinningObserver.observe(true);
	audioManager::init();
	audioSourceManager::init();
	renderThreadReady.exchange(false);
	renderThread = new tbb::tbb_thread(renderThreadFunc);
	// pm = new _physicsManager();

	while (!renderThreadReady.load())
		this_thread::sleep_for(1ms);
	waitForRenderJob([]() {});

	// root = new Transform(0);
	root2 = Transforms._new();
	setRootGameObject(root2);

	transformIdThreadcache = vector<vector<vector<int>>>(concurrency::numThreads, vector<vector<int>>(3));
	positionsToBuffer = vector<vector<glm::vec3>>(concurrency::numThreads);
	rotationsToBuffer = vector<vector<glm::quat>>(concurrency::numThreads);
	scalesToBuffer = vector<vector<glm::vec3>>(concurrency::numThreads);

	m_editor = new editor();
	m_editor->c.fov = radians(80.f);
	m_editor->c.farPlane = 1e32f;
	m_editor->c.nearPlane = 0.00001f;

	// load_game("");

	// auto default_cube = new game_object();
	// default_cube->addComponent<_renderer>()->set(_shader("res/shaders/model.vert", "res/shaders/model.frag"),_model("res/models/cube/cube.obj"));

	// game_object *player = new game_object();
	// player->transform.name() = "editor";
	// auto playerCam = player->addComponent<_camera>();
	// playerCam->fov = 80;
	// playerCam->farPlane = 1e32f;
	// playerCam->nearPlane = 0.00001f;
	// player->addComponent<editor_sc>();//->ammo_proto = game_object_prototype(bomb_proto);
}

void physicsUpdate(float dt)
{
	timer stopWatch;
	static float time = 0;
	time += dt;
	if (time > 1.f / 30.f)
	{

		componentStorage<collider> *cb = COMPONENT_LIST(collider);
		stopWatch.start();

		parralelfor(cb->size(), {
			if (cb->data.valid[i])
			{
				cb->data.data[i]._update();
			}
		});
		appendStat(cb->name + "--update", stopWatch.stop());
		stopWatch.start();
		_parallel_for(cb->data, [&](int i) {
			if (cb->data.valid[i])
			{
				cb->data.data[i].midUpdate();
			}
		});
		appendStat(cb->name + "--mid_update", stopWatch.stop());
		stopWatch.start();
		parralelfor(cb->size(), {
			if (cb->data.valid[i])
			{
				cb->data.data[i]._lateUpdate();
			}
		});
		appendStat(cb->name + "--late_update", stopWatch.stop());
		time -= (1.f / 30.f);
	}
}
void run()
{
	StartComponents(COMPONENT_LIST(_renderer));
	StartComponents(COMPONENT_LIST(terrain));

	timer stopWatch;
	timer gameLoopTotal;
	timer gameLoopMain;
	timer frameCap;
	int32 entities = 0;
	float tot = gameLoopTotal.stop();

	while (!glfwWindowShouldClose(window) && Time.time < maxGameDuration)
	{

		function<void()> *f;
		while (mainThreadWork.try_pop(f))
		{
			(*f)();
			delete f;
			/* code */
		}
		//  (auto &i : mainThreadWork)
		// {
		// 	(*i)();
		// 	delete i;
		// }
		// mainThreadWork.clear();
		transformLock.lock();

		gameLoopTotal.start();
		gameLoopMain.start();
		{

			// unique_lock l(transformLock);
			// scripting
			if (isGameRunning())
			{
				doLoopIteration(ComponentRegistry.components);
				for (auto &i : physics_manager::collisionGraph)
					physics_manager::collisionLayers[i.first].clear();
				// doLoopIteration(gameEngineComponents, false);

				/////////////////////////////////////////////////
				physicsUpdate(Time.unscaledDeltaTime);
			}
			if (COMPONENT_LIST(_camera)->size())
				audioManager::updateListener(COMPONENT_LIST(_camera)->get(0)->transform->getPosition());

			stopWatch.start();
			tbb::parallel_for_each(toDestroyGameObjects.range(), [](game_object *g) { g->_destroy(); });
			toDestroyGameObjects.clear();

			entities = Transforms.getCount();
			transformLock.unlock();
		}
		appendStat("destroy deffered", stopWatch.stop());
		appendStat("game loop main", gameLoopMain.stop());

		auto cameras = COMPONENT_LIST(_camera);
		auto colliders = COMPONENT_LIST(collider);

		if (COMPONENT_LIST(_camera)->size())
		{
			glm::vec3 fo = cameras->get(0)->transform.getPosition();
			if (glm::length2(fo) > 1e80)
			{
				floating_origin.push(fo);
				_parallel_for(Transforms, [&](int i) {
					Transforms.positions[i] -= fo;
					Transforms.updates[i].pos = true;
				});
				_parallel_for(*colliders, [&](int i) {
					if (colliders->data.valid[i] && colliders->data.data[i].type == colType::pointType)
					{
						colliders->data.data[i].p.pos1 -= fo;
					}
				});
			}
			else
			{
				floating_origin.push(vec3(0));
			}
		}
		else
		{
			floating_origin.push(vec3(0));
		}

		waitForRenderJob([]() { updateTiming(); });
		waitForRenderJob([]() { batchManager::updateBatches(); });
		stopWatch.start();
		renderLock.lock();
		appendStat("wait for render", stopWatch.stop());

		// this_thread::sleep_for((1000.f / 30.f - frameCap.stop()) * 1ms);
		// frameCap.start();
		transformsBuffered.store(false);
		////////////////////////////////////// update camera data for frame ///////////////////

		for (_camera &c : cameras->data.data)
		{
			c.c->update(c.transform->getPosition(), c.transform.getRotation());
		}

		////////////////////////////////////// set up transforms/renderer data to buffer //////////////////////////////////////
		stopWatch.start();
		int __renderersSize = 0;
		// int __rendererOffsetsSize = 0;
		__renderer_offsets->storage->clear();
		__rendererMetas->storage->clear();
		__renderMeta rm;
		// rm.min = 0;
		// rm.max = 1e32f;
		for (auto &i : batchManager::batches.back())
		{
			for (auto &j : i.second)
			{
				for (auto &k : j.second)
				{
					__renderer_offsets->storage->push_back(__renderersSize);
					rm.radius = k.first->m.meta()->radius;
					rm.isBillboard = k.first->isBillboard;
					rm.min = k.first->minRadius;
					rm.max = k.first->maxRadius;
					__rendererMetas->storage->push_back(rm);
					__renderersSize += k.first->ids.size();
				}
			}
		}
		__RENDERERS_in->storage->resize(__renderersSize);

		tbb::parallel_for(
			tbb::blocked_range<unsigned int>(0, concurrency::numThreads, 1),
			[&](const tbb::blocked_range<unsigned int> &r) {
				for (unsigned int id = r.begin(); id < r.end(); ++id)
				{

					int numt = concurrency::numThreads;
					int step = Transforms.size() / concurrency::numThreads;
					int i = step * id;

					transformIdThreadcache[id][0].clear(); // pos
					transformIdThreadcache[id][1].clear(); // scl
					transformIdThreadcache[id][2].clear(); // rot

					positionsToBuffer[id].clear();
					rotationsToBuffer[id].clear();
					scalesToBuffer[id].clear();

					auto from = Transforms.updates.begin() + step * id;
					auto to = from + step;

					// transformIdThreadcache[id].reserve(step + 1);
					if (id == concurrency::numThreads - 1)
						to = Transforms.updates.end();
					while (from != to)
					{
						if (from->pos)
						{
							from->pos = false;
							transformIdThreadcache[id][0].emplace_back(i);
							positionsToBuffer[id].emplace_back(((transform2)i).getPosition());
						}
						if (from->rot)
						{
							from->rot = false;
							transformIdThreadcache[id][1].emplace_back(i);
							rotationsToBuffer[id].emplace_back(((transform2)i).getRotation());
						}
						if (from->scl)
						{
							from->scl = false;
							transformIdThreadcache[id][2].emplace_back(i);
							scalesToBuffer[id].emplace_back(((transform2)i).getScale());
						}
						++from;
						++i;
					}
					// }

					int __rendererId = 0;
					int __rendererOffset = 0;
					typename vector<__renderer>::iterator __r = __RENDERERS_in->storage->begin();
					for (auto &i : batchManager::batches.back())
					{
						for (auto &j : i.second)
						{
							for (auto &k : j.second)
							{
								int step = k.first->ids.size() / concurrency::numThreads;
								typename deque<GLuint>::iterator from = k.first->ids.data.begin() + step * id;
								typename deque<GLuint>::iterator to = from + step;
								__r = __RENDERERS_in->storage->begin() + __rendererOffset + step * id;
								if (id == concurrency::numThreads - 1)
								{
									to = k.first->ids.data.end();
								}
								while (from != to)
								{
									__r->transform = *from;
									__r->id = __rendererId;
									++from;
									++__r;
								}
								++__rendererId;
								__rendererOffset += k.first->ids.size();
							}
						}
					}
				}
			});
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
		if (Input.getKeyDown(GLFW_KEY_B))
		{
			cameras->data.data.front().c->lockFrustum = !cameras->data.data.front().c->lockFrustum;
		}

		renderJob *rj = new renderJob();
		rj->work = [&] { return; };
		rj->type = renderNum::render;
		renderDone.store(false);

		renderWork.push(rj);
		renderLock.unlock();

		tot = gameLoopTotal.stop();
		appendStat("game loop total", tot);
	}
	delete m_editor;

	// log("end of program");
	waitForRenderJob([&]() {});

	// concurrency::pinningObserver.observe(false);

	rootGameObject->destroy();
	destroyAllComponents();
	audioManager::destroy();

	renderJob *rj = new renderJob();
	rj->type = rquit;
	renderLock.lock();
	renderWork.push(rj);
	renderLock.unlock();
	while (renderThreadReady.load())
		this_thread::sleep_for(1ms);
	renderThread->join();

	// pm->destroy();
	// delete pm;
	cout << endl;
	componentStats.erase("");
	for (map<string, rolling_buffer>::iterator i = componentStats.begin(); i != componentStats.end(); ++i)
	{
		cout << i->first << " -- avg: " << i->second.getAverageValue() << " -- stdDev: " << i->second.getStdDeviation() << endl;
	}
	cout << "fps : " << 1.f / Time.unscaledSmoothDeltaTime << endl;
	cout << "entities : " << entities << endl;
}
