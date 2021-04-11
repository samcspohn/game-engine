#include "helper1.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <climits>
const GLint WIDTH = 1920, HEIGHT = 1080;
int SCREEN_WIDTH, SCREEN_HEIGHT;
thread::id renderThreadID;

void log(string log)
{
	std::cout << log << std::endl;
}

Barrier::Barrier(std::size_t count) : m_count{count}, m_initial{count}, m_state{State::Down} {}

/// Blocks until all N threads reach here
void Barrier::Wait()
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

rolling_buffer::rolling_buffer()
{
	size = 100;
}
rolling_buffer::rolling_buffer(int size)
{
	this->size = size;
}
deque<double> buffer;
double runningTotal = 0;
void rolling_buffer::add(double timeDelta)
{
	buffer.push_back(timeDelta);
	runningTotal += timeDelta;
	if (buffer.size() > size)
	{
		runningTotal -= buffer.front();
		buffer.erase(buffer.begin());
	}
}
float rolling_buffer::getAverageValue()
{
	return runningTotal / buffer.size();
}
float rolling_buffer::getStdDeviation()
{
	double standardDeviation = 0.0;
	float mean = getAverageValue();
	for (auto &i : buffer)
	{
		standardDeviation += pow(i - mean, 2);
	}
	return glm::sqrt(standardDeviation / buffer.size());
}
_time Time;

void timer::start()
{
	tstart = high_resolution_clock::now();
}
// return time in milliseconds since start
float timer::stop()
{
	return (float)duration_cast<microseconds>(high_resolution_clock::now() - tstart).count() / 1000.0;
}

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
void seedRand(glm::uvec3 s)
{
	x = s.x;
	y = s.y;
	z = s.z;
}

mutex randMutex;
float randf()
{
	// randMutex.lock();
	float ret = (float)xorshf96() / (float)ULONG_MAX;
	// randMutex.unlock();
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

std::mutex rdr_lck;
bool renderRunning = true;
std::queue<std::shared_ptr<std::function<void()>>> renderJobs;

void _enqueRenderJob(std::function<void()> *work)
{
	// renderJob *rj = new renderJob();
	// rj->type = renderNum::doFunc;
	// rj->work = [work]() { work(); };
	// rj->completed = -1;
	auto job = std::shared_ptr<std::function<void()>>(work);
	rdr_lck.lock();
	renderJobs.push(job);
	rdr_lck.unlock();
}

void _waitForRenderJob(std::function<void()> *work)
{
	// renderJob *rj = new renderJob();
	// rj->type = renderNum::doFunc;
	// rj->work = [work]() { work(); };
	// rj->completed = 0;

	if (renderThreadID == this_thread::get_id())
	{
		(*work)();
	}
	else
	{
		auto job = std::shared_ptr<std::function<void()>>(work);
		rdr_lck.lock();
		renderJobs.push(job);
		rdr_lck.unlock();

		while (!job.unique())
		{
			this_thread::sleep_for(1ns);
		}
	}
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

gpuTimer::gpuTimer()
{
	// startTime = -1;
	// stopTime = -1;
	// queryID[0] = -1;
	// queryID[1] = -1;
}
void gpuTimer::start()
{
	// t.start();
	// generate two queries

	glGenQueries(2, queryID);
	glQueryCounter(queryID[0], GL_TIMESTAMP);
}
float gpuTimer::stop()
{
	// return -1;
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
	glDeleteQueries(2, queryID);
	return (float)(stopTime - startTime) / 1000000.0;
}

string to_string(glm::vec3 v)
{
	return to_string(v.x) + "," + to_string(v.y) + "," + to_string(v.z);
}