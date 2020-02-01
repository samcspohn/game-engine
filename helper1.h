#pragma once

#include <iostream>

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
using namespace std;
using namespace chrono;
#ifndef HELPER
#define HELPER


struct rolling_buffer
{
	rolling_buffer() {
		size = 100;
	}
	rolling_buffer(int size) {
		this->size = size;
	}
	deque<double> buffer;
	double runningTotal = 0;
	void add(double timeDelta) {
		buffer.push_back(timeDelta);
		runningTotal += timeDelta;
		if (buffer.size() > size) {
			runningTotal -= buffer.front();
			buffer.erase(buffer.begin());
		}
	}
	float getAverageValue() {
		return runningTotal / buffer.size();
	}
private:
	int size;
};


class _time {
public:
	double time;
	double unscaledTime;
	float deltaTime;
	float unscaledDeltaTime;
	double timeScale = 1;
	rolling_buffer timeBuffer = rolling_buffer(20);
	float smoothDeltaTime;
	float unscaledSmoothDeltaTime;
private:
};

_time Time;
class timer{
    high_resolution_clock::time_point tstart;
public:
    void start(){
        tstart = high_resolution_clock::now();
    }
    float stop(){
        return (float)duration_cast<microseconds>(high_resolution_clock::now() - tstart).count();
    }
};


//
//extern _time& Time = _time::_T;

vector<string> splitString(string s_, char delim);

enum renderNum { render, doFunc, rquit };
struct renderJob {
	renderNum type;
	int val1;
	glm::mat4 proj;
	glm::mat4 rot;
	glm::mat4 view;
	bool completed;
	std::function<void(void)> work = 0;
};


void waitFor(bool& cond) {
	while (!cond)
		this_thread::sleep_for(1ns);
}

void waitFor(atomic<bool>& cond) {
	while (!cond)
		this_thread::sleep_for(1ns);
}
static unsigned long x=123456789, y=362436069, z=521288629;

unsigned long xorshf96(void) {          //period 2^96-1
unsigned long t;
    x ^= x << 16;
    x ^= x >> 5;
    x ^= x << 1;

   t = x;
   x = y;
   y = z;
   z = t ^ x ^ y;

  return z;
}

mutex randMutex;
float randf() {
	randMutex.lock();
	float ret = (float)xorshf96() / (float)ULONG_MAX;
	randMutex.unlock();
	return ret;
}
int _min(int a, int b) {
	if (a < b)
		return a;
	return b;
}

int _max(int a, int b) {
	if (a > b)
		return a;
	return b;
}

float _min(float a, float b) {
	if (a < b)
		return a;
	return b;
}

float _max(float a, float b) {
	if (a > b)
		return a;
	return b;
}
glm::vec3 randomSphere() {
	glm::vec3 d = glm::normalize(glm::vec3(randf() * 2 - 1.0f, randf() * 2 - 1.0f, randf() * 2 - 1.0f));
	float c =  powf(randf(), 1.0f / 3.0f);
	return d * c;
}


vector<string> splitString(string s_, char delim) {
	string s;
	stringstream ss(s_);
	vector<string> ret;
	while (getline(ss, s, delim)) {
		ret.push_back(s);
	}
	return ret;
}



std::mutex renderLock;
std::queue<renderJob> renderWork = std::queue<renderJob>();

void enqueRenderJob(void (*work)(void) ) {
	renderJob rj;
	rj.type = renderNum::doFunc;
	rj.work = [work]() {work(); };

	renderLock.lock();
	renderWork.push(rj);
	renderLock.unlock();
}
#endif // !HELPER
