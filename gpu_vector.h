#pragma once

#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
// #include <SOIL2.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <sstream>
#include <mutex>
#include <atomic>
#include <algorithm>
#include "helper1.h"
using namespace std;

template<typename t>
class gpu_vector {
public:
	gpu_vector() {
	}
	void init() {
		renderLock.lock();
		renderJob rj;
		maxSize = 1;
		rj.type = doFunc;
		rj.work = [&]() { _init(); };
		renderWork.push(rj);
		renderLock.unlock();
		//_init();
	}
	void ownStorage() {
		if (ownsStorage)
			throw;
		storage = new vector<t>();
		ownsStorage = true;
	}
	void _init() {
		glGenBuffers(1, &bufferId);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferId);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(t) * maxSize, 0, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		inited = true;
	}
	~gpu_vector() {
		glDeleteBuffers(1, &bufferId);
		if (ownsStorage)
			delete storage;
	}
	bool inited = false;
	mutex lock;
	GLint maxSize = 1;
	GLuint bufferId;
	GLuint binding;
	vector<t>* storage;

	void realloc() {
		glDeleteBuffers(1, &bufferId);
		glGenBuffers(1, &bufferId);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferId);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(t) * maxSize, 0, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}
	void bufferData() {
		lock.lock();
		if (!inited)
			_init();
		bool reallocBuff = false;
		while ((float)storage->size() > (float)maxSize) {
			maxSize *= 2;
			reallocBuff = true;
		}
		if (reallocBuff)
			realloc();
		lock.unlock();
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferId);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(t) * storage->size(), storage->data());//buffer data
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}



	bool tryRealloc() {
		bool ret = false;
		if (!inited) {
			_init();
			ret = true;
		}
		bool reallocBuff = false;
		while ((float)storage->size() > (float)maxSize) {
			maxSize *= 2;
			reallocBuff = true;
			ret = true;
		}
		if (reallocBuff)
			realloc();
		return ret;
	}

	GLuint size() {
		return storage->size();
	}

private:
	bool ownsStorage = false;
};


template<typename t>
class gpu_vector_proxy {
public:
	gpu_vector_proxy() {
	}
	void init() {
		renderLock.lock();
		renderJob rj;
		rj.type = doFunc;
		rj.work = [&]() { _init(); };
		renderWork.push(rj);
		renderLock.unlock();
		//_init();
	}

	void _init() {
		glGenBuffers(1, &bufferId);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferId);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(t) * maxSize, 0, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		inited = true;
	}
	~gpu_vector_proxy() {
		glDeleteBuffers(1, &bufferId);
	}
	bool inited = false;
	mutex lock;
	GLint maxSize = 1;
	GLuint bufferId;
	GLuint binding;

	void realloc() {
		glDeleteBuffers(1, &bufferId);
		glGenBuffers(1, &bufferId);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferId);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(t) * maxSize, 0, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}
	void bufferData(vector<t>& data) {
		lock.lock();
		if (!inited)
			_init();
		bool reallocBuff = false;
		while (data.size() > (float)maxSize) {
			maxSize *= 2;
			reallocBuff = true;
		}
		if (reallocBuff)
			realloc();
		lock.unlock();
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferId);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(t) * data.size(), data.data());//buffer data
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	bool tryRealloc(size_t size) {
		bool ret = false;
		if (!inited) {
			_init();
			ret = true;
		}
		bool reallocBuff = false;
		while (size > (float)maxSize) {
			maxSize *= 2;
			reallocBuff = true;
			ret = true;
		}
		if (reallocBuff)
			realloc();
		return ret;
	}
};
