#pragma once

#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <sstream>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <functional>
#include <deque>
#include <queue>
#include <thread>
#include <chrono>
#include <map>
#include <condition_variable>
#include "concurrency.h"
using namespace std;
using namespace chrono;
#ifndef HELPER
#define HELPER

extern const GLint WIDTH, HEIGHT;
extern int SCREEN_WIDTH, SCREEN_HEIGHT;
extern thread::id renderThreadID;

void log(string log = "");
template <class T>
std::string FormatWithCommas(T value)
{
	std::stringstream ss;
	ss.imbue(std::locale(""));
	ss << std::fixed << value;
	return ss.str();
}

class Barrier
{
private:
	std::mutex m_mutex;
	std::condition_variable m_cv;

	size_t m_count;
	const size_t m_initial;

	enum State : unsigned char
	{
		Up,
		Down
	};
	State m_state;

public:
	explicit Barrier(std::size_t count);

	/// Blocks until all N threads reach here
	void Wait();
};
struct rolling_buffer
{
	rolling_buffer();
	rolling_buffer(int size);
	deque<double> buffer;
	double runningTotal = 0;
	void add(double timeDelta);
	float getAverageValue();
	float getStdDeviation();

private:
	int size;
};

class _time
{
public:
	double time;
	double unscaledTime;
	float deltaTime = 0.0001;
	float unscaledDeltaTime;
	double timeScale = 1;
	rolling_buffer timeBuffer = rolling_buffer(20);
	float smoothDeltaTime;
	float unscaledSmoothDeltaTime;

private:
};

extern _time Time;
class timer
{
	high_resolution_clock::time_point tstart;

public:
	void start();
	float stop();
};

//
//extern _time& Time = _time::_T;

vector<string> splitString(string s_, char delim);

enum renderNum
{
	render,
	doFunc,
	rquit
};
struct renderJob
{
	renderNum type;
	int completed;
	std::function<void(void)> work = 0;
};

void waitFor(bool &cond);
void waitFor(atomic<bool> &cond);
unsigned long xorshf96(void);

void seedRand(glm::uvec3 s);
float randf();
int _min(int a, int b);
int _max(int a, int b);
float _min(float a, float b);
float _max(float a, float b);
glm::vec3 randomSphere();

vector<string> splitString(string s_, char delim);

extern std::mutex rdr_lck;
extern bool renderRunning;
extern std::queue<std::shared_ptr<std::function<void()>>> renderJobs;

#define enqueRenderJob(work)                              \
	{                                                     \
		_enqueRenderJob(new std::function<void()>(work)); \
	}

#define waitForRenderJob(work)                              \
	{                                                       \
		_waitForRenderJob(new std::function<void()>(work)); \
	}

void _enqueRenderJob(std::function<void(void)> *work);
void _waitForRenderJob(std::function<void(void)> *work);

extern map<string, rolling_buffer> componentStats;

extern mutex statLock;
void appendStat(string name, float dtime);

class gpuTimer
{
	// timer t;
	GLuint64 startTime, stopTime;
	unsigned int queryID[2];

public:
	gpuTimer();
	void start();
	float stop();
};

// template <typename t, typename u>
// void _parallel_for(t &T, u U)
// {
// 	int size = T.size();
// 	int grain = size / concurrency::numThreads / concurrency::numThreads;
// 	grain = glm::max(grain, 1);
// 	tbb::parallel_for(
// 		tbb::blocked_range<unsigned int>(0, size, grain),
// 		[&](const tbb::blocked_range<unsigned int> &r) {
// 			for (unsigned int i = r.begin(); i < r.end(); ++i)
// 			{
// 				U(i);
// 			}
// 		}
// 		// ,
// 		// update_ap
// 	);
// }

// #include <tbb/parallel_for.h>
// #define parallelfor(size, func)                                               \
// 	{                                                                         \
// 		int grain = size / concurrency::numThreads / concurrency::numThreads; \
// 		grain = glm::max(grain, 1);                                           \
// 		tbb::parallel_for(                                                    \
// 			tbb::blocked_range<unsigned int>(0, size, grain),                 \
// 			[&](const tbb::blocked_range<unsigned int> &r) {                  \
// 				for (unsigned int i = r.begin(); i < r.end(); ++i)            \
// 				{                                                             \
// 					func                                                      \
// 				}                                                             \
// 			});                                                               \
// 	}

#define parallelfor(size, func)                                               \
	{                                                                         \
		int grain = size / concurrency::numThreads / concurrency::numThreads; \
		grain = glm::max(grain, 1);                                           \
		concurrency::_parallelfor.doWork(                                     \
			size, [&](int i) { func }, grain);                                \
	}

#endif // !HELPER

string to_string(glm::vec3 v);