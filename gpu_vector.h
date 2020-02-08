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
#include <map>

using namespace std;

class gpu_vector_base{
	protected:
	static atomic<int> idGenerator;
	public:
	virtual void deleteBuffer() = 0;
};
atomic<int> gpu_vector_base::idGenerator = 0;
map<int,gpu_vector_base*> gpu_buffers;

template<typename t>
class gpu_vector : public gpu_vector_base{
public:
	gpu_vector() {
		id = this->gpu_vector_base::idGenerator.fetch_add(1);
		gpu_buffers.insert(std::pair(id,this));
	}
	t& operator[](unsigned int i) {
		return storage->at(i);
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

	void bindData(uint index){
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, bufferId);
	}
	void retrieveData(){
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferId);
    	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(t) * size(), storage->data());
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	void deleteBuffer(){
		gpu_buffers.erase(id);
		delete this;
	}

	int id;
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
	~gpu_vector() {
		glDeleteBuffers(1, &bufferId);
		if (ownsStorage)
			delete storage;
	}
	bool ownsStorage = false;
};


template<typename t>
class gpu_vector_proxy : public gpu_vector_base{
		~gpu_vector_proxy() {
		glDeleteBuffers(1, &bufferId);
	}
public:
	gpu_vector_proxy() {
		id = this->gpu_vector_base::idGenerator.fetch_add(1);
		gpu_buffers.insert(std::pair(id,this));
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
	void bindData(uint index){
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, bufferId);
	}

	void deleteBuffer(){
		gpu_buffers.erase(id);
		delete this;
	}
	int id;
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
