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
#include <condition_variable>
using namespace std;
using namespace chrono;
#ifndef HELPER
#define HELPER
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
	explicit Barrier(std::size_t count) : m_count{count}, m_initial{count}, m_state{State::Down} {}

	/// Blocks until all N threads reach here
	void Wait()
	{
		std::unique_lock<std::mutex> lock{m_mutex};

		if (m_state == State::Down)
		{
			// Counting down the number of syncing threads
			if (--m_count == 0)
			{
				m_state = State::Up;
				m_cv.notify_all();
			}
			else
			{
				m_cv.wait(lock, [this] { return m_state == State::Up; });
			}
		}

		else // (m_state == State::Up)
		{
			// Counting back up for Auto reset
			if (++m_count == m_initial)
			{
				m_state = State::Down;
				m_cv.notify_all();
			}
			else
			{
				m_cv.wait(lock, [this] { return m_state == State::Down; });
			}
		}
	}
};
struct rolling_buffer
{
	rolling_buffer()
	{
		size = 100;
	}
	rolling_buffer(int size)
	{
		this->size = size;
	}
	deque<double> buffer;
	double runningTotal = 0;
	void add(double timeDelta)
	{
		buffer.push_back(timeDelta);
		runningTotal += timeDelta;
		if (buffer.size() > size)
		{
			runningTotal -= buffer.front();
			buffer.erase(buffer.begin());
		}
	}
	float getAverageValue()
	{
		return runningTotal / buffer.size();
	}
	float getStdDeviation()
	{
		double standardDeviation = 0.0;
		float mean = getAverageValue();
		for (auto &i : buffer)
		{
			standardDeviation += pow(i - mean, 2);
		}
		return glm::sqrt(standardDeviation / buffer.size());
	}

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

_time Time;
class timer
{
	high_resolution_clock::time_point tstart;

public:
	void start()
	{
		tstart = high_resolution_clock::now();
	}
	float stop()
	{
		return (float)duration_cast<microseconds>(high_resolution_clock::now() - tstart).count() / 1000.0;
	}
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
	// renderJob(const renderJob& rj){
	// 	this->type = rj.type;
	// 	this->completed = -1;
	// 	this->work = 0;
	// }
	// renderJob(){}
	renderNum type;
	int completed;
	std::function<void(void)> work = 0;
};

void waitFor(bool &cond)
{
	while (!cond)
		this_thread::sleep_for(1ns);
}

void waitFor(atomic<bool> &cond)
{
	while (!cond)
		this_thread::sleep_for(1ns);
}
static unsigned long x = 123456789, y = 362436069, z = 521288629;

unsigned long xorshf96(void)
{ //period 2^96-1
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
float randf()
{
	randMutex.lock();
	float ret = (float)xorshf96() / (float)ULONG_MAX;
	randMutex.unlock();
	return ret;
}
int _min(int a, int b)
{
	if (a < b)
		return a;
	return b;
}

int _max(int a, int b)
{
	if (a > b)
		return a;
	return b;
}

float _min(float a, float b)
{
	if (a < b)
		return a;
	return b;
}

float _max(float a, float b)
{
	if (a > b)
		return a;
	return b;
}
glm::vec3 randomSphere()
{
	glm::vec3 d = glm::normalize(glm::vec3(randf() * 2 - 1.0f, randf() * 2 - 1.0f, randf() * 2 - 1.0f));
	float c = powf(randf(), 1.0f / 3.0f);
	return d * c;
}

vector<string> splitString(string s_, char delim)
{
	string s;
	stringstream ss(s_);
	vector<string> ret;
	while (getline(ss, s, delim))
	{
		ret.push_back(s);
	}
	return ret;
}

std::mutex renderLock;
std::queue<renderJob *> renderWork = std::queue<renderJob *>();

inline void enqueRenderJob(std::function<void(void)> work)
{
	renderJob *rj = new renderJob();
	rj->type = renderNum::doFunc;
	rj->work = [work]() { work(); };
	rj->completed = -1;
	renderLock.lock();
	renderWork.push(rj);
	renderLock.unlock();
}

inline void waitForRenderJob(std::function<void(void)> work)
{
	renderJob *rj = new renderJob();
	rj->type = renderNum::doFunc;
	rj->work = [work]() { work(); };
	rj->completed = 0;

	renderLock.lock();
	renderWork.push(rj);
	renderLock.unlock();

	while (rj->completed != 1)
	{
		this_thread::sleep_for(1ns);
	}
	rj->completed = 2;
}

map<string, rolling_buffer> componentStats;

mutex statLock;
void appendStat(string name, float dtime)
{
	statLock.lock();
	auto a = componentStats.find(name);
	if (a != componentStats.end())
		a->second.add(dtime);
	else
	{
		componentStats[name] = rolling_buffer(200);
		componentStats[name].add(dtime);
	}
	statLock.unlock();
}

class gpuTimer
{
	// timer t;
	GLuint64 startTime, stopTime;
	unsigned int queryID[2];

public:
	gpuTimer()
	{
		startTime = -1;
		stopTime = -1;
		queryID[0] = -1;
		queryID[1] = -1;
	}
	void start()
	{
		// t.start();
		// generate two queries

		glGenQueries(2, queryID);
		glQueryCounter(queryID[0], GL_TIMESTAMP);
	}
	float stop()
	{
		// glFlush();
		// return t.stop();
		glQueryCounter(queryID[1], GL_TIMESTAMP);
		// wait until the results are available
		GLint stopTimerAvailable = 0;
		while (!stopTimerAvailable)
		{
			glGetQueryObjectiv(queryID[1],
							   GL_QUERY_RESULT_AVAILABLE,
							   &stopTimerAvailable);
		}

		// get query results
		glGetQueryObjectui64v(queryID[0], GL_QUERY_RESULT, &startTime);
		glGetQueryObjectui64v(queryID[1], GL_QUERY_RESULT, &stopTime);
		return (float)(stopTime - startTime) / 1000000.0;
	}
};

#endif // !HELPER
