#pragma once

#include <iostream>

// #include "/usr/include/GL/glew.h"
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
#include "../helper1.h"
#include <map>

using namespace std;


class gpu_vector_base{
	protected:
	static atomic<int> idGenerator;
	public:
	virtual void deleteBuffer() = 0;
	
};
extern map<int,gpu_vector_base*> gpu_buffers;

template<typename t>
class gpu_vector : public gpu_vector_base{
public:
	gpu_vector() {
		usage = GL_DYNAMIC_DRAW;
		id = this->gpu_vector_base::idGenerator.fetch_add(1);
		gpu_buffers.insert(std::pair<int,gpu_vector_base*>(id,this));
	}
	t& operator[](unsigned int i) {
		return (*storage)[i];
	}
	t& at(uint i){
		return storage->at(i);
	}
	void init() {
		enqueRenderJob([&]() { _init(); });
	}
	void ownStorage() {
		if (ownsStorage)
			return;
		storage = new vector<t>();
		ownsStorage = true;
	}
	void logErr(string err){
		cout << "gpu_vector " << id << " type: " << typeid(t).name() << " err: " << err << endl;
	}
	void _init() {
		glGenBuffers(1, &bufferId);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferId);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(t) * maxSize, 0, usage);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		inited = true;
	}

	void bindData(uint index){
		if(!inited){
			_init();
		}
		glGetError();
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, bufferId);
		GLenum err = glGetError();
		if(err)
			logErr("bind " + to_string(err));
		
	}
	void retrieveData(){
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferId);
    	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(t) * size(), storage->data());
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
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
	GLuint bufferId = -1;
	GLenum usage;
	vector<t>* storage;

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
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
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

	void realloc() {
		// glDeleteBuffers(1, &bufferId);
		// glGenBuffers(1, &bufferId);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferId);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(t) * maxSize, 0, usage);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
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
		usage = GL_DYNAMIC_DRAW;
		id = this->gpu_vector_base::idGenerator.fetch_add(1);
		gpu_buffers.insert(std::pair<int,gpu_vector_base*>(id,this));
	}
	void init() {
		enqueRenderJob([&]() { _init(); });
	}

	void logErr(string err){
		cout << "gpu_vector " << id << " type: " << typeid(t).name() << " err: " << err << endl;
	}

	void _init() {
		glGetError();
		glGenBuffers(1, &bufferId);
		if(bufferId == -1){
			logErr("bad init");
		}
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferId);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(t) * maxSize, 0, usage);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		GLenum err = glGetError();
		if(err)
			logErr("init " + to_string(err));
		inited = true;
	}
	void bindData(uint index){
		if(!inited){
			_init();
		}
		glGetError();
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, bufferId);
		GLenum err = glGetError();
		if(err)
			logErr("bind " + to_string(err));
	}

	void deleteBuffer(){
		gpu_buffers.erase(id);
		delete this;
	}
	int id;
	bool inited = false;
	mutex lock;
	GLint maxSize = 1;
	GLuint bufferId = -1;
	GLenum usage;


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
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}
	void bufferData(vector<t>& data, GLuint offset,GLuint size){
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferId);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(t) * offset, sizeof(t) * size, data.data());//buffer data
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	void bufferData(t* data, GLuint offset,GLuint size){
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferId);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(t) * offset, sizeof(t) * size, data);//buffer data
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	void retrieveData(vector<t>& storage){
		storage.resize(maxSize);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferId);
    	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(t) * maxSize, storage.data());
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}
	void retrieveData(vector<t>& storage, int _size){
		storage.resize(_size);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferId);
    	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(t) * _size, storage.data());
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}
	void grow(size_t size){
		if (!inited) {
			_init();
		}
		bool reallocBuff = false;
		GLint _size = maxSize;
		while (size > (float)maxSize) {
			maxSize *= 2;
			reallocBuff = true;
		}
		if (reallocBuff){
			realloc(_size);
		}
	}

	// increases max size if size is greater and copys contents to new buffer
	void resize(size_t size){
		grow(size);
	}

	// increases max size if size is greater.
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
		if (reallocBuff){
			realloc();
		}
		return ret;
	}

private:
	void realloc() {
		glGetError();
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferId);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(t) * maxSize, 0, usage);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		GLenum err = glGetError();
		if(err)
		logErr("realloc" + to_string(err));
	}
	void realloc(uint oldSize) {
		GLuint newId;
		glGenBuffers(1, &newId);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, newId);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(t) * maxSize, 0, usage);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		
		glBindBuffer(GL_COPY_READ_BUFFER,bufferId);
		glBindBuffer(GL_COPY_WRITE_BUFFER,newId);
		glCopyBufferSubData(GL_COPY_READ_BUFFER,GL_COPY_WRITE_BUFFER,0,0,sizeof(t)*oldSize);

		glDeleteBuffers(1, &bufferId);
		this->bufferId = newId;
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	}
};
